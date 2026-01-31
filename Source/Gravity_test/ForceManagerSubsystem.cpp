#include "ForceManagerSubsystem.h"

#include "ForceConsumerInterface.h"
#include "GravitySourceBase.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

UForceManagerSubsystem::UForceManagerSubsystem()
{
}

void UForceManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("✅ ForceManagerSubsystem Initialized!"));
}

void UForceManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
	AccMap.Empty();
	ActorAccMap.Empty();
	TouchedReceivers.Empty();
	TouchedActors.Empty();
	Sources.Empty();
}

void UForceManagerSubsystem::Tick(float DeltaTime)
{
	if (!bEnabled || !GetWorld())
	{
		return;
	}

	ResolveAndApply(DeltaTime);
	ClearPerTickState();
	PruneSources();
}

TStatId UForceManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UForceManagerSubsystem, STATGROUP_Tickables);
}

bool UForceManagerSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return bEnabled && World && World->IsGameWorld();
}

UWorld* UForceManagerSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

void UForceManagerSubsystem::RegisterSource(AGravitySourceBase* Source)
{
	if (!IsValid(Source))
	{
		return;
	}

	Sources.AddUnique(Source);
	UE_LOG(LogTemp, Warning, TEXT("✅ Source Registered: %s"), *Source->GetName());
}

void UForceManagerSubsystem::UnregisterSource(AGravitySourceBase* Source)
{
	if (!Source)
	{
		return;
	}

	Sources.Remove(Source);
}

void UForceManagerSubsystem::AddContribution(UPrimitiveComponent* ReceiverComp, const FVector& Accel, uint32 TypeMask, FName DebugName)
{
	(void)TypeMask;
	(void)DebugName;

	if (!IsValid(ReceiverComp) || !ReceiverComp->IsSimulatingPhysics())
	{
		return;
	}

	FForceAccumulator& Accumulator = AccMap.FindOrAdd(ReceiverComp);
	if (!Accumulator.bTouched)
	{
		Accumulator.bTouched = true;
		TouchedReceivers.Add(ReceiverComp);
	}

	Accumulator.SumAccel += Accel;
	UE_LOG(LogTemp, Warning, TEXT("✅ AddContribution: Receiver=%s, Accel=%s"), *ReceiverComp->GetName(), *Accel.ToString());
}

void UForceManagerSubsystem::AddContributionActor(AActor* ReceiverActor, const FVector& Accel, uint32 TypeMask, FName DebugName)
{
	(void)TypeMask;
	(void)DebugName;

	if (!IsValid(ReceiverActor))
	{
		return;
	}

	FForceAccumulator& Accumulator = ActorAccMap.FindOrAdd(ReceiverActor);
	if (!Accumulator.bTouched)
	{
		Accumulator.bTouched = true;
		TouchedActors.Add(ReceiverActor);
	}

	Accumulator.SumAccel += Accel;
}

const FReceiverForceSettings& UForceManagerSubsystem::GetDefaultSettings() const
{
	return DefaultSettings;
}

FVector UForceManagerSubsystem::ResolveFinalAccel(const FForceAccumulator& Accumulator, float DeltaTime) const
{
	(void)DeltaTime;

	FVector FinalAccel = Accumulator.SumAccel;

	const float MinAccelSquared = DefaultSettings.MinAccel * DefaultSettings.MinAccel;
	if (FinalAccel.SizeSquared() < MinAccelSquared)
	{
		return FVector::ZeroVector;
	}

	const float MaxAccelSquared = DefaultSettings.MaxAccel * DefaultSettings.MaxAccel;
	const float CurrentSquared = FinalAccel.SizeSquared();
	if (CurrentSquared > MaxAccelSquared)
	{
		const float CurrentLength = FMath::Sqrt(CurrentSquared);
		if (CurrentLength > KINDA_SMALL_NUMBER)
		{
			FinalAccel *= (DefaultSettings.MaxAccel / CurrentLength);
		}
	}

	return FinalAccel;
}

void UForceManagerSubsystem::ApplyToPhysics(UPrimitiveComponent* Comp, const FVector& FinalAccel) const
{
	if (!IsValid(Comp) || !Comp->IsSimulatingPhysics())
	{
		return;
	}

	if (FinalAccel.IsNearlyZero())
	{
		return;
	}

	switch (DefaultSettings.MassMode)
	{
		case EMassMode::Physical:
		{
			const float Mass = Comp->GetMass();
			const FVector Force = FinalAccel * Mass;
			Comp->AddForce(Force);
			break;
		}
		case EMassMode::AccelChange:
		{
			Comp->AddForce(FinalAccel, NAME_None, true);
			break;
		}
		case EMassMode::Hybrid:
		default:
			break;
	}
}

void UForceManagerSubsystem::ResolveAndApply(float DeltaTime)
{
	for (int32 Index = 0; Index < TouchedReceivers.Num(); ++Index)
	{
		TWeakObjectPtr<UPrimitiveComponent> ReceiverPtr = TouchedReceivers[Index];
		UPrimitiveComponent* ReceiverComp = ReceiverPtr.Get();
		if (!IsValid(ReceiverComp))
		{
			AccMap.Remove(ReceiverPtr);
			continue;
		}

		FForceAccumulator* Accumulator = AccMap.Find(ReceiverPtr);
		if (!Accumulator)
		{
			continue;
		}

		const FVector FinalAccel = ResolveFinalAccel(*Accumulator, DeltaTime);
		ApplyToPhysics(ReceiverComp, FinalAccel);
		AccMap.Remove(ReceiverPtr);
	}

	for (int32 Index = 0; Index < TouchedActors.Num(); ++Index)
	{
		TWeakObjectPtr<AActor> ActorPtr = TouchedActors[Index];
		AActor* ReceiverActor = ActorPtr.Get();
		if (!IsValid(ReceiverActor))
		{
			ActorAccMap.Remove(ActorPtr);
			continue;
		}

		FForceAccumulator* Accumulator = ActorAccMap.Find(ActorPtr);
		if (!Accumulator)
		{
			continue;
		}

		const FVector FinalAccel = ResolveFinalAccel(*Accumulator, DeltaTime);
		if (ReceiverActor->GetClass()->ImplementsInterface(UForceConsumerInterface::StaticClass()))
		{
			IForceConsumerInterface::Execute_OnForceResolved(ReceiverActor, FinalAccel, DeltaTime);
		}
		ActorAccMap.Remove(ActorPtr);
	}
}

void UForceManagerSubsystem::ClearPerTickState()
{
	TouchedReceivers.Reset();
	TouchedActors.Reset();
}

void UForceManagerSubsystem::PruneSources()
{
	for (int32 Index = Sources.Num() - 1; Index >= 0; --Index)
	{
		if (!Sources[Index].IsValid())
		{
			Sources.RemoveAtSwap(Index);
		}
	}
}
