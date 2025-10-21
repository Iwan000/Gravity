#include "GravityWellActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
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
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

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

    VisualizationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualizationMesh"));
    VisualizationMesh->SetupAttachment(SceneRoot);
    VisualizationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualizationMesh->SetGenerateOverlapEvents(false);
    VisualizationMesh->SetCastShadow(false);
    VisualizationMesh->bHiddenInGame = true;
    VisualizationMesh->SetCanEverAffectNavigation(false);
    VisualizationMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        VisualizationMesh->SetStaticMesh(SphereMesh.Object);
    }

    AccretionVfxComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AccretionVFX"));
    AccretionVfxComponent->SetupAttachment(SceneRoot);
    AccretionVfxComponent->SetAutoActivate(false);
    AccretionVfxComponent->SetCanEverAffectNavigation(false);
}

void AGravityWellActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateSphereRadius();
    PulseAccumulator = 0.f;
    RefreshVisualizationAssets();
    UpdateVisualizationActivation();
    UpdateVisualizationScale();
    UpdateVisualizationParameters(0.f);
}

void AGravityWellActor::BeginPlay()
{
    Super::BeginPlay();
    UpdateSphereRadius();
    StartGravityTimer();
    PulseAccumulator = 0.f;
    RefreshVisualizationAssets();
    UpdateVisualizationActivation();
    UpdateVisualizationScale();
    UpdateVisualizationParameters(0.f);
}

void AGravityWellActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AccretionVfxComponent)
    {
        AccretionVfxComponent->DeactivateImmediate();
    }
    VisualizationMID = nullptr;
    RestoreAllCharacters();
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AGravityWellActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    UpdateSphereRadius();
    PulseAccumulator = 0.f;
    RefreshVisualizationAssets();
    UpdateVisualizationActivation();
    UpdateVisualizationScale();
    UpdateVisualizationParameters(0.f);
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

    UpdateVisualizationActivation();
    UpdateVisualizationScale();
    UpdateVisualizationParameters(DeltaSeconds);

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

void AGravityWellActor::RefreshVisualizationAssets()
{
    if (VisualizationMesh)
    {
        if (bEnableVisualization && VisualizationMaterial)
        {
            VisualizationMesh->SetMaterial(0, VisualizationMaterial);
        }

        if (UMaterialInterface* ActiveMaterial = VisualizationMesh->GetMaterial(0))
        {
            VisualizationMID = VisualizationMesh->CreateDynamicMaterialInstance(0, ActiveMaterial);
        }
        else
        {
            VisualizationMID = nullptr;
        }
    }

    if (AccretionVfxComponent)
    {
        if (AccretionNiagaraSystem)
        {
            const UNiagaraSystem* CurrentSystem = AccretionVfxComponent->GetAsset();
            if (CurrentSystem != AccretionNiagaraSystem)
            {
                AccretionVfxComponent->SetAsset(AccretionNiagaraSystem);
            }
        }
    }
}

void AGravityWellActor::UpdateVisualizationActivation()
{
    const bool bShouldShow = bEnableVisualization && (VisualizationMesh != nullptr);

    if (VisualizationMesh)
    {
        VisualizationMesh->SetHiddenInGame(!bShouldShow);
        VisualizationMesh->SetVisibility(bShouldShow, true);
    }

    if (AccretionVfxComponent)
    {
        if (bEnableVisualization && AccretionNiagaraSystem)
        {
            if (!AccretionVfxComponent->IsActive())
            {
                AccretionVfxComponent->Activate();
            }
        }
        else
        {
            AccretionVfxComponent->DeactivateImmediate();
        }
    }
}

void AGravityWellActor::UpdateVisualizationScale()
{
    if (!VisualizationMesh || VisualizationMeshReferenceRadius <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float TargetScale = MaxRadius / VisualizationMeshReferenceRadius;
    VisualizationMesh->SetRelativeScale3D(FVector(TargetScale));

    if (AccretionVfxComponent)
    {
        AccretionVfxComponent->SetRelativeScale3D(FVector(TargetScale));
    }
}

void AGravityWellActor::UpdateVisualizationParameters(float DeltaSeconds)
{
    if (!bEnableVisualization)
    {
        if (AccretionVfxComponent && AccretionVfxComponent->IsActive())
        {
            AccretionVfxComponent->DeactivateImmediate();
        }
        return;
    }

    if (VisualizationMID)
    {
        if (!RadiusParameterName.IsNone())
        {
            VisualizationMID->SetScalarParameterValue(RadiusParameterName, MaxRadius);
        }

        if (!StrengthParameterName.IsNone())
        {
            VisualizationMID->SetScalarParameterValue(StrengthParameterName, Strength);
        }

        if (!PulseParameterName.IsNone())
        {
            const float SafeSpeed = FMath::Max(PulseSpeed, 0.f);
            PulseAccumulator += DeltaSeconds * SafeSpeed;
            if (PulseAccumulator > 1000.f)
            {
                PulseAccumulator = FMath::Fmod(PulseAccumulator, 1.f);
            }
            const float Phase = (SafeSpeed <= KINDA_SMALL_NUMBER)
                ? 0.f
                : FMath::Fmod(PulseAccumulator, 1.f);
            VisualizationMID->SetScalarParameterValue(PulseParameterName, Phase * PulseIntensity);
        }
    }

    if (AccretionVfxComponent)
    {
        if (AccretionNiagaraSystem)
        {
            if (!RadiusParameterName.IsNone())
            {
                AccretionVfxComponent->SetFloatParameter(RadiusParameterName, MaxRadius);
            }
            if (!StrengthParameterName.IsNone())
            {
                AccretionVfxComponent->SetFloatParameter(StrengthParameterName, Strength);
            }
            if (!PulseParameterName.IsNone())
            {
                const float PulseValue = PulseIntensity;
                AccretionVfxComponent->SetFloatParameter(PulseParameterName, PulseValue);
            }
        }

            if (!AccretionVfxComponent->IsActive() && AccretionNiagaraSystem)
            {
                AccretionVfxComponent->Activate();
            }
        }
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
