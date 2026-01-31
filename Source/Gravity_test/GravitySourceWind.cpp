#include "GravitySourceWind.h"

#include "Components/PrimitiveComponent.h"

AGravitySourceWind::AGravitySourceWind()
{
	SourceType = EForceType::Wind;
}

FVector AGravitySourceWind::CalculateContribution(UPrimitiveComponent* ReceiverComp, float DeltaTime) const
{
	(void)ReceiverComp;
	(void)DeltaTime;

	const FVector Direction = bUseActorForward ? GetActorForwardVector() : WindDirection;
	const FVector UnitDirection = Direction.GetSafeNormal();
	if (UnitDirection.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	return UnitDirection * Strength;
}
