#include "GravitySourceWhiteHole.h"

#include "Components/PrimitiveComponent.h"

AGravitySourceWhiteHole::AGravitySourceWhiteHole()
{
	SourceType = EForceType::WhiteHole;
}

FVector AGravitySourceWhiteHole::CalculateContribution(UPrimitiveComponent* ReceiverComp, float DeltaTime) const
{
	UE_UNUSED(DeltaTime);

	if (!IsValid(ReceiverComp))
	{
		return FVector::ZeroVector;
	}

	const FVector SourceLocation = GetActorLocation();
	const FVector ReceiverLocation = ReceiverComp->GetComponentLocation();
	FVector Direction = ReceiverLocation - SourceLocation;
	const float Distance = Direction.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	Direction /= Distance;
	const float EffectiveDistance = FMath::Max(Distance, MinRadius);
	float AccelMagnitude = Strength / (EffectiveDistance * EffectiveDistance);

	if (MaxAccel > 0.0f)
	{
		AccelMagnitude = FMath::Min(AccelMagnitude, MaxAccel);
	}

	return Direction * AccelMagnitude;
}
