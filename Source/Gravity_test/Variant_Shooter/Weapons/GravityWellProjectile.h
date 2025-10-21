#pragma once

#include "ShooterProjectile.h"
#include "GravityWellProjectile.generated.h"

class AGravityWellActor;

DECLARE_MULTICAST_DELEGATE_OneParam(FGravityWellProjectileActivatedSignature, class AGravityWellProjectile*);
DECLARE_MULTICAST_DELEGATE_OneParam(FGravityWellProjectileDeactivatedSignature, class AGravityWellProjectile*);

/**
 * Projectile that can transform into a stationary gravity well on demand.
 */
UCLASS()
class GRAVITY_TEST_API AGravityWellProjectile : public AShooterProjectile
{
	GENERATED_BODY()

public:
	AGravityWellProjectile();

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Manually convert the projectile into a stationary gravity well. */
	UFUNCTION(BlueprintCallable, Category="Gravity Well")
	void ActivateBlackHole();

	/** Removes the spawned gravity well and destroys this projectile. */
	UFUNCTION(BlueprintCallable, Category="Gravity Well")
	void DeactivateBlackHole();

	/** Returns whether the projectile is currently acting as a gravity well. */
	UFUNCTION(BlueprintPure, Category="Gravity Well")
	bool IsBlackHoleActive() const { return bBlackHoleActive; }

	/** Broadcast when the projectile successfully activates the gravity well. */
	FGravityWellProjectileActivatedSignature OnBlackHoleActivated;

	/** Broadcast when the projectile removes the gravity well (or is destroyed). */
	FGravityWellProjectileDeactivatedSignature OnBlackHoleDeactivated;

protected:
	/** Class of gravity well actor to spawn on activation. */
	UPROPERTY(EditAnywhere, Category="Gravity Well")
	TSubclassOf<AGravityWellActor> GravityWellClass;

	/** Optional offset applied when spawning the gravity well actor. */
	UPROPERTY(EditAnywhere, Category="Gravity Well")
	FVector WellSpawnOffset = FVector::ZeroVector;

	/** If true the projectile destroys itself automatically when the well is deactivated. */
	UPROPERTY(EditAnywhere, Category="Gravity Well")
	bool bDestroyProjectileWithWell = true;

	/** Blueprint hook fired when activation completes. */
	UFUNCTION(BlueprintImplementableEvent, Category="Gravity Well", meta=(DisplayName="On Black Hole Activated"))
	void BP_OnBlackHoleActivated();

	/** Blueprint hook fired when deactivation completes. */
	UFUNCTION(BlueprintImplementableEvent, Category="Gravity Well", meta=(DisplayName="On Black Hole Deactivated"))
	void BP_OnBlackHoleDeactivated();

private:
	void SpawnGravityWell();
	void DestroyGravityWell();

	UFUNCTION()
	void HandleWellDestroyed(AActor* DestroyedActor);

	/** Tracks whether the projectile has already become a gravity well. */
	bool bBlackHoleActive = false;

	/** Pointer to the spawned gravity well actor, if any. */
	TWeakObjectPtr<AGravityWellActor> ActiveWell;
};
