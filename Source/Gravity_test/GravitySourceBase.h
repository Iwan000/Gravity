#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForceTypes.h"
#include "GravitySourceBase.generated.h"

class UShapeComponent;
class UPrimitiveComponent;
class UForceManagerSubsystem;

UCLASS(Abstract, Blueprintable)
class GRAVITY_TEST_API AGravitySourceBase : public AActor
{
	GENERATED_BODY()

public:
	AGravitySourceBase();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual FVector CalculateContribution(UPrimitiveComponent* ReceiverComp, float DeltaTime) const PURE_VIRTUAL(AGravitySourceBase::CalculateContribution, return FVector::ZeroVector;);

	bool IsValidReceiver(UPrimitiveComponent* Comp) const;
	bool IsSpecialReceiver(AActor* Actor) const;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditAnywhere, Category = "Force")
	FName SourceId;

	UPROPERTY(EditAnywhere, Category = "Force")
	EForceType SourceType = EForceType::Other;

	UPROPERTY(EditAnywhere, Category = "Force")
	float Strength = 3000000.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Force")
	TObjectPtr<UShapeComponent> RangeCollision;

	TSet<TWeakObjectPtr<UPrimitiveComponent>> ReceiversInRange;
	TSet<TWeakObjectPtr<AActor>> SpecialReceiversInRange;

	TWeakObjectPtr<UForceManagerSubsystem> ManagerRef;
};
