#pragma once

#include "GravityWellProjectile.h"
#include "WhiteHoleProjectile.generated.h"

/**
 * Projectile variant that spawns a white-hole (repelling) gravity actor.
 */
UCLASS()
class GRAVITY_TEST_API AWhiteHoleProjectile : public AGravityWellProjectile
{
	GENERATED_BODY()

public:
	AWhiteHoleProjectile();
};
