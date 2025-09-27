#include "GravityWellActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "WorldCollision.h"
#include "Misc/Optional.h"

namespace
{
    constexpr float KMinimumTickInterval = 0.005f;
}

DEFINE_LOG_CATEGORY(LogGravityWell);

AGravityWellActor::AGravityWellActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    InfluenceSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InfluenceSphere"));
    InfluenceSphere->SetupAttachment(SceneRoot);
    InfluenceSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InfluenceSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    InfluenceSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InfluenceSphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    InfluenceSphere->SetGenerateOverlapEvents(true);
    InfluenceSphere->bHiddenInGame = true;
}

void AGravityWellActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateSphereRadius();
}

void AGravityWellActor::BeginPlay()
{
    Super::BeginPlay();
    UpdateSphereRadius();
    StartGravityTimer();
}

void AGravityWellActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RestoreAllCharacters();
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AGravityWellActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    UpdateSphereRadius();
}
#endif

void AGravityWellActor::UpdateSphereRadius()
{
    const float SafeRadius = FMath::Max(MaxRadius, 0.f);
    InfluenceSphere->SetSphereRadius(SafeRadius, true);
}

void AGravityWellActor::StartGravityTimer()
{
    if (!ensure(TickInterval >= KMinimumTickInterval))
    {
        TickInterval = 0.03f;
    }

    GetWorldTimerManager().ClearTimer(GravityTimerHandle);
    GetWorldTimerManager().SetTimer(GravityTimerHandle, this, &AGravityWellActor::ApplyGravityTick, TickInterval, true, 0.f);
}

void AGravityWellActor::ApplyGravityTick()
{
    if (!InfluenceSphere)
    {
        return;
    }

    const FVector WellLocation = InfluenceSphere->GetComponentLocation();
    const float DeltaSeconds = FMath::Max(GravityTimerHandle.IsValid()
        ? GetWorldTimerManager().GetTimerRate(GravityTimerHandle)
        : TickInterval, KINDA_SMALL_NUMBER);

    TSet<TWeakObjectPtr<ACharacter>> CurrentlyOverlappingCharacters;
    TArray<UPrimitiveComponent*> OverlappingComponents;

    if (UWorld* World = GetWorld())
    {
        FCollisionObjectQueryParams ObjectParams;
        ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
        ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
        ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
        ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GravityWellOverlap), false, this);
        QueryParams.bReturnPhysicalMaterial = false;

        const float SphereRadius = InfluenceSphere->GetScaledSphereRadius();
        const FCollisionShape SphereShape = FCollisionShape::MakeSphere(SphereRadius);

        TArray<FOverlapResult> Overlaps;
        if (World->OverlapMultiByObjectType(Overlaps, WellLocation, FQuat::Identity, ObjectParams, SphereShape, QueryParams))
        {
            for (const FOverlapResult& Overlap : Overlaps)
            {
                if (UPrimitiveComponent* Primitive = Overlap.Component.Get())
                {
                    OverlappingComponents.AddUnique(Primitive);
                }
            }
        }
    }

    UE_LOG(LogGravityWell, Log, TEXT("%s ticking with %d overlapping components"), *GetName(), OverlappingComponents.Num());

    for (UPrimitiveComponent* Primitive : OverlappingComponents)
    {
        if (!Primitive)
        {
            continue;
        }

        if (Primitive->GetOwner() == this)
        {
            continue;
        }

        const FVector TargetLocation = Primitive->GetComponentLocation();
        const FVector Accel = ComputeAcceleration(WellLocation, TargetLocation);

        if (!Accel.IsNearlyZero())
        {
            if (bAffectRigidBodies && Primitive->IsSimulatingPhysics())
            {
                Primitive->WakeAllRigidBodies();
                const float Mass = Primitive->GetMass();
                if (Mass > KINDA_SMALL_NUMBER)
                {
                    Primitive->AddForce(Accel * Mass, NAME_None, true);
                    UE_LOG(LogGravityWell, Log, TEXT("Applied accel %s to %s (mass %.2f)"), *Accel.ToString(), *Primitive->GetName(), Mass);
                }
            }

            if (bAffectCharacters)
            {
                if (AActor* OwningActor = Primitive->GetOwner())
                {
                    if (ACharacter* Character = Cast<ACharacter>(OwningActor))
                    {
                        CurrentlyOverlappingCharacters.Add(Character);

                        if (!AffectedCharacters.Contains(Character))
                        {
                            if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
                            {
                                if (!FindCharacterState(Character))
                                {
                                    FAffectedCharacterState NewState;
                                    NewState.Character = Character;
                                    NewState.PreviousGravityScale = MoveComp->GravityScale;
                                    NewState.PreviousMovementMode = static_cast<uint8>(MoveComp->MovementMode);
                                    CharacterStates.Add(NewState);
                                }

                                MoveComp->GravityScale = 0.f;
                                MoveComp->SetMovementMode(MOVE_Flying);
                                UE_LOG(LogGravityWell, Log, TEXT("%s entering gravity well; stored gravity %.2f mode %d"), *Character->GetName(), MoveComp->GravityScale, MoveComp->MovementMode);
                            }
                            AffectedCharacters.Add(Character);
                        }

                        if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
                        {
                            MoveComp->Velocity += Accel * DeltaSeconds;
                            MoveComp->UpdateComponentVelocity();
                            UE_LOG(LogGravityWell, Log, TEXT("Applied character accel %s to %s; new velocity %s"), *Accel.ToString(), *Character->GetName(), *MoveComp->Velocity.ToString());
                        }
                    }
                }
            }
        }
    }

    // Restore gravity for characters no longer affected.
    for (auto It = AffectedCharacters.CreateIterator(); It; ++It)
    {
        TWeakObjectPtr<ACharacter>& CharacterPtr = *It;
        if (!CharacterPtr.IsValid() || !CurrentlyOverlappingCharacters.Contains(CharacterPtr))
        {
            RestoreCharacterGravity(CharacterPtr);
            It.RemoveCurrent();
        }
    }
}

void AGravityWellActor::RestoreCharacterGravity(TWeakObjectPtr<ACharacter> CharacterPtr)
{
    if (!CharacterPtr.IsValid())
    {
        RemoveCharacterState(CharacterPtr);
        return;
    }

    if (UCharacterMovementComponent* MoveComp = CharacterPtr->GetCharacterMovement())
    {
        if (FAffectedCharacterState* State = FindCharacterState(CharacterPtr))
        {
            MoveComp->GravityScale = State->PreviousGravityScale;
            MoveComp->SetMovementMode(static_cast<EMovementMode>(State->PreviousMovementMode));
            UE_LOG(LogGravityWell, Log, TEXT("%s exiting gravity well; restored gravity %.2f mode %d"), *CharacterPtr->GetName(), State->PreviousGravityScale, State->PreviousMovementMode);
        }
        else
        {
            MoveComp->GravityScale = 1.f;
            MoveComp->SetMovementMode(MOVE_Walking);
            UE_LOG(LogGravityWell, Log, TEXT("%s exiting gravity well with default restore"), *CharacterPtr->GetName());
        }
    }

    RemoveCharacterState(CharacterPtr);
}

void AGravityWellActor::RestoreAllCharacters()
{
    for (TWeakObjectPtr<ACharacter> CharacterPtr : AffectedCharacters)
    {
        RestoreCharacterGravity(CharacterPtr);
    }
    AffectedCharacters.Reset();
    CharacterStates.Reset();
}

FVector AGravityWellActor::ComputeAcceleration(const FVector& WellLocation, const FVector& TargetLocation) const
{
    const FVector Delta = WellLocation - TargetLocation;
    const float Distance = Delta.Size();

    if (Distance > MaxRadius)
    {
        return FVector::ZeroVector;
    }

    if (Distance <= KINDA_SMALL_NUMBER)
    {
        return FVector::ZeroVector;
    }

    const float ClampedRadius = FMath::Max(Distance, MinRadius);
    FVector Direction = Delta.GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }

    const float AccelMagnitude = Strength / FMath::Max(ClampedRadius * ClampedRadius, 1.f);
    FVector Accel = Direction * AccelMagnitude;
    return Accel.GetClampedToMaxSize(MaxAccel);
}

FAffectedCharacterState* AGravityWellActor::FindCharacterState(const TWeakObjectPtr<ACharacter>& CharacterPtr)
{
    for (FAffectedCharacterState& State : CharacterStates)
    {
        if (!State.Character.IsValid())
        {
            continue;
        }
        if (State.Character == CharacterPtr)
        {
            return &State;
        }
    }
    return nullptr;
}

void AGravityWellActor::RemoveCharacterState(const TWeakObjectPtr<ACharacter>& CharacterPtr)
{
    CharacterStates.RemoveAllSwap([&CharacterPtr](const FAffectedCharacterState& State)
    {
        return !State.Character.IsValid() || State.Character == CharacterPtr;
    });
}
