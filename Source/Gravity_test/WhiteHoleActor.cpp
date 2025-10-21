#include "WhiteHoleActor.h"

FVector AWhiteHoleActor::ComputeAcceleration(const FVector& WellLocation, const FVector& TargetLocation) const
{
    // Invert the gravitational pull from the base implementation to push actors away.
    return -Super::ComputeAcceleration(WellLocation, TargetLocation);
}

