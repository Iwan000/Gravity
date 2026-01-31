#pragma once

#include "CoreMinimal.h"
#include "ForceTypes.generated.h"

UENUM(BlueprintType)
enum class EForceType : uint8
{
	BlackHole UMETA(DisplayName = "Black Hole"),
	WhiteHole UMETA(DisplayName = "White Hole"),
	Wind UMETA(DisplayName = "Wind"),
	Other UMETA(DisplayName = "Other")
};

UENUM(BlueprintType)
enum class EMassMode : uint8
{
	Physical UMETA(DisplayName = "Physical"),
	AccelChange UMETA(DisplayName = "Accel Change"),
	Hybrid UMETA(DisplayName = "Hybrid")
};

FORCEINLINE uint32 ForceTypeToMask(EForceType ForceType)
{
	return 1u << static_cast<uint8>(ForceType);
}
