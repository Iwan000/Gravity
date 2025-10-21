#pragma once

#include "ShooterWeapon.h"
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

protected:
	virtual void FireProjectile(const FVector& TargetLocation) override;

private:
	void BindToProjectile(AGravityWellProjectile* Projectile);

	void PromotePendingIfActivated();

	void ClearProjectileReferences(AGravityWellProjectile* Projectile);

	UFUNCTION()
	void OnTrackedProjectileDestroyed(AActor* DestroyedActor);

	void HandleProjectileActivated(AGravityWellProjectile* Projectile);
	void HandleProjectileDeactivated(AGravityWellProjectile* Projectile);

	/** Projectile currently travelling and waiting for activation. */
	TWeakObjectPtr<AGravityWellProjectile> PendingProjectile;

	/** Projectile that has already converted into an active gravity well. */
	TWeakObjectPtr<AGravityWellProjectile> ActiveProjectile;
};
