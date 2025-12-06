#pragma once

#include "ShooterWeapon.h"
#include <cfloat>
#include "GravityWellWeapon.generated.h"

class AGravityWellProjectile;

/**
 * Weapon variant that fires gravity-well projectiles with manual activation.
 */
UCLASS()
class GRAVITY_TEST_API AGravityWellWeapon : public AShooterWeapon
{
	GENERATED_BODY()

public:
	AGravityWellWeapon();

	virtual void StartFiring() override;
	virtual void StopFiring() override;
	virtual void StartSecondaryFire() override;
	virtual void StopSecondaryFire() override;

protected:
	virtual void BeginPlay() override;

private:
	void HandlePrimaryFire();
	void HandleSecondaryFire();

	void ActivateFlyingProjectile();
	AGravityWellProjectile* SpawnGravityProjectile();

	void BindProjectileDelegates(AGravityWellProjectile* Projectile);

	UFUNCTION()
	void HandleProjectileDestroyed(AActor* DestroyedActor);

	void HandleProjectileActivated(AGravityWellProjectile* Projectile);
	void HandleProjectileDeactivated(AGravityWellProjectile* Projectile);

	UPROPERTY(EditAnywhere, Category="Gravity Well")
	TSubclassOf<AGravityWellProjectile> BlackHoleProjectileClass;

	TWeakObjectPtr<AGravityWellProjectile> FlyingProjectile;
	TWeakObjectPtr<AGravityWellProjectile> ActiveHoleProjectile;

	float LastShotTime = -FLT_MAX;
};
