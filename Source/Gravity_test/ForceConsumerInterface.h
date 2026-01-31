#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ForceConsumerInterface.generated.h"

UINTERFACE(Blueprintable)
class GRAVITY_TEST_API UForceConsumerInterface : public UInterface
{
	GENERATED_BODY()
};

class GRAVITY_TEST_API IForceConsumerInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Force")
	void OnForceResolved(const FVector& FinalAccel, float DeltaTime);
};
