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
	void HandleTriggerPressed(bool bIsBlackHole);

	void PromotePendingIfActivated(bool bIsBlackHole);

	AGravityWellProjectile* SpawnGravityProjectile(TSubclassOf<AGravityWellProjectile> ProjectileClass, bool bIsBlackHole);

	void BindProjectileDelegates(AGravityWellProjectile* Projectile, bool bIsBlackHole);
	void ClearProjectilePointers(AGravityWellProjectile* Projectile, bool bIsBlackHole);

	UFUNCTION()
	void OnBlackProjectileDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnWhiteProjectileDestroyed(AActor* DestroyedActor);

	void HandleBlackProjectileActivated(AGravityWellProjectile* Projectile);
	void HandleBlackProjectileDeactivated(AGravityWellProjectile* Projectile);
	void HandleWhiteProjectileActivated(AGravityWellProjectile* Projectile);
	void HandleWhiteProjectileDeactivated(AGravityWellProjectile* Projectile);

	UPROPERTY(EditAnywhere, Category="Gravity Well")
	TSubclassOf<AGravityWellProjectile> BlackHoleProjectileClass;

	UPROPERTY(EditAnywhere, Category="Gravity Well")
	TSubclassOf<AGravityWellProjectile> WhiteHoleProjectileClass;

	TWeakObjectPtr<AGravityWellProjectile> PendingBlackProjectile;
	TWeakObjectPtr<AGravityWellProjectile> ActiveBlackProjectile;

	TWeakObjectPtr<AGravityWellProjectile> PendingWhiteProjectile;
	TWeakObjectPtr<AGravityWellProjectile> ActiveWhiteProjectile;

	float LastBlackShotTime = -FLT_MAX;
	float LastWhiteShotTime = -FLT_MAX;
};
