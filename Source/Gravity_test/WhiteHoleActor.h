#pragma once

#include "CoreMinimal.h"
#include "GravityWellActor.h"
#include "WhiteHoleActor.generated.h"

/**
 * White hole variant that repels overlapping actors instead of attracting them.
 */
UCLASS(Blueprintable)
class GRAVITY_TEST_API AWhiteHoleActor : public AGravityWellActor
{
    GENERATED_BODY()

protected:
    virtual FVector ComputeAcceleration(const FVector& WellLocation, const FVector& TargetLocation) const override;
};

