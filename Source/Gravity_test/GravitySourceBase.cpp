#include "GravitySourceBase.h"

#include "ForceConsumerInterface.h"
#include "ForceManagerSubsystem.h"
#include "Components/ShapeComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

AGravitySourceBase::AGravitySourceBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	USphereComponent* SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RangeCollision"));
	SphereComponent->InitSphereRadius(1500.0f);
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComponent->SetCollisionObjectType(ECC_WorldDynamic);
	SphereComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
	SphereComponent->SetGenerateOverlapEvents(true);
	SphereComponent->SetCanEverAffectNavigation(false);
	RangeCollision = SphereComponent;
	SetRootComponent(RangeCollision);
}

void AGravitySourceBase::BeginPlay()
{
	Super::BeginPlay();

	if (SourceId.IsNone())
	{
		SourceId = GetFName();
	}

	if (RangeCollision)
	{
		RangeCollision->OnComponentBeginOverlap.AddDynamic(this, &AGravitySourceBase::HandleBeginOverlap);
		RangeCollision->OnComponentEndOverlap.AddDynamic(this, &AGravitySourceBase::HandleEndOverlap);
	}

	if (UWorld* World = GetWorld())
	{
		ManagerRef = World->GetSubsystem<UForceManagerSubsystem>();
		if (ManagerRef.IsValid())
		{
			ManagerRef->RegisterSource(this);
		}
	}
}

void AGravitySourceBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ManagerRef.IsValid())
	{
		ManagerRef->UnregisterSource(this);
	}

	ReceiversInRange.Reset();
	SpecialReceiversInRange.Reset();

	Super::EndPlay(EndPlayReason);
}

void AGravitySourceBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ManagerRef.IsValid())
	{
		return;
	}

	const uint32 TypeMask = ForceTypeToMask(SourceType);

	for (auto It = ReceiversInRange.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		UPrimitiveComponent* ReceiverComp = It->Get();
		if (!IsValid(ReceiverComp))
		{
			It.RemoveCurrent();
			continue;
		}

		const FVector Accel = CalculateContribution(ReceiverComp, DeltaTime);
		if (!Accel.IsNearlyZero())
		{
			ManagerRef->AddContribution(ReceiverComp, Accel, TypeMask, SourceId);
		}
	}

	for (auto It = SpecialReceiversInRange.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		AActor* ReceiverActor = It->Get();
		if (!IsValid(ReceiverActor))
		{
			It.RemoveCurrent();
			continue;
		}

		UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(ReceiverActor->GetRootComponent());
		if (!RootPrimitive)
		{
			RootPrimitive = ReceiverActor->FindComponentByClass<UPrimitiveComponent>();
		}

		if (!RootPrimitive)
		{
			continue;
		}

		const FVector Accel = CalculateContribution(RootPrimitive, DeltaTime);
		if (!Accel.IsNearlyZero())
		{
			ManagerRef->AddContributionActor(ReceiverActor, Accel, TypeMask, SourceId);
		}
	}
}

bool AGravitySourceBase::IsValidReceiver(UPrimitiveComponent* Comp) const
{
	if (!IsValid(Comp))
	{
		return false;
	}

	if (Comp->GetOwner() == this)
	{
		return false;
	}

	return Comp->IsSimulatingPhysics();
}

bool AGravitySourceBase::IsSpecialReceiver(AActor* Actor) const
{
	if (!IsValid(Actor) || Actor == this)
	{
		return false;
	}

	return Actor->GetClass()->ImplementsInterface(UForceConsumerInterface::StaticClass());
}

void AGravitySourceBase::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_UNUSED(OverlappedComponent);
	UE_UNUSED(OtherBodyIndex);
	UE_UNUSED(bFromSweep);
	UE_UNUSED(SweepResult);

	if (!OtherActor || OtherActor == this || !OtherComp)
	{
		return;
	}

	if (IsValidReceiver(OtherComp))
	{
		ReceiversInRange.Add(OtherComp);
		return;
	}

	if (IsSpecialReceiver(OtherActor))
	{
		SpecialReceiversInRange.Add(OtherActor);
	}
}

void AGravitySourceBase::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UE_UNUSED(OverlappedComponent);
	UE_UNUSED(OtherBodyIndex);

	if (!OtherActor || OtherActor == this || !OtherComp)
	{
		return;
	}

	ReceiversInRange.Remove(OtherComp);

	if (IsSpecialReceiver(OtherActor) && RangeCollision)
	{
		if (!RangeCollision->IsOverlappingActor(OtherActor))
		{
			SpecialReceiversInRange.Remove(OtherActor);
		}
	}
}
