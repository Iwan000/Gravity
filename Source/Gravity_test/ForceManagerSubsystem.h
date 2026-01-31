#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "ForceTypes.h"
#include "ForceManagerSubsystem.generated.h"

class AActor;
class AGravitySourceBase;
class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct FReceiverForceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Force")
	float MinAccel = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Force")
	float MaxAccel = 999999.0f;

	UPROPERTY(EditAnywhere, Category = "Force")
	EMassMode MassMode = EMassMode::Physical;
};

struct FForceAccumulator
{
	FVector SumAccel = FVector::ZeroVector;
	bool bTouched = false;
};

UCLASS()
class GRAVITY_TEST_API UForceManagerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UForceManagerSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;

	void RegisterSource(class AGravitySourceBase* Source);
	void UnregisterSource(class AGravitySourceBase* Source);

	void AddContribution(UPrimitiveComponent* ReceiverComp, const FVector& Accel, uint32 TypeMask, FName DebugName = NAME_None);
	void AddContributionActor(AActor* ReceiverActor, const FVector& Accel, uint32 TypeMask, FName DebugName = NAME_None);

	const FReceiverForceSettings& GetDefaultSettings() const;

private:
	FVector ResolveFinalAccel(const FForceAccumulator& Accumulator, float DeltaTime) const;
	void ApplyToPhysics(UPrimitiveComponent* Comp, const FVector& FinalAccel) const;
	void ResolveAndApply(float DeltaTime);
	void ClearPerTickState();
	void PruneSources();

	TArray<TWeakObjectPtr<AGravitySourceBase>> Sources;

	TMap<TWeakObjectPtr<UPrimitiveComponent>, FForceAccumulator> AccMap;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> TouchedReceivers;

	TMap<TWeakObjectPtr<AActor>, FForceAccumulator> ActorAccMap;
	TArray<TWeakObjectPtr<AActor>> TouchedActors;

	UPROPERTY(EditAnywhere, Category = "Force")
	FReceiverForceSettings DefaultSettings;

	bool bEnabled = true;
};
