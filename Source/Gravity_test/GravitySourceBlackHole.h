#pragma once

#include "CoreMinimal.h"
#include "GravitySourceBase.h"
#include "GravitySourceBlackHole.generated.h"

UCLASS(Blueprintable)
class GRAVITY_TEST_API AGravitySourceBlackHole : public AGravitySourceBase
{
	GENERATED_BODY()

public:
	AGravitySourceBlackHole();

protected:
	UPROPERTY(EditAnywhere, Category = "Force", meta = (ClampMin = "1.0"))
	float MinRadius = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Force", meta = (ClampMin = "0.0"))
	float MaxAccel = 0.0f;

	virtual FVector CalculateContribution(UPrimitiveComponent* ReceiverComp, float DeltaTime) const override;
};
