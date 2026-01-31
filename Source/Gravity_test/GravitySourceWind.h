#pragma once

#include "CoreMinimal.h"
#include "GravitySourceBase.h"
#include "GravitySourceWind.generated.h"

UCLASS(Blueprintable)
class GRAVITY_TEST_API AGravitySourceWind : public AGravitySourceBase
{
	GENERATED_BODY()

public:
	AGravitySourceWind();

protected:
	UPROPERTY(EditAnywhere, Category = "Force")
	bool bUseActorForward = true;

	UPROPERTY(EditAnywhere, Category = "Force", meta = (EditCondition = "!bUseActorForward"))
	FVector WindDirection = FVector(1.0f, 0.0f, 0.0f);

	virtual FVector CalculateContribution(UPrimitiveComponent* ReceiverComp, float DeltaTime) const override;
};
