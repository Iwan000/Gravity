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

	/** Transform an existing black hole into a white hole at the same location. */
	UFUNCTION(BlueprintCallable, Category="Gravity Well")
	void TransformToWhiteHole();

	/** Removes the spawned gravity well and destroys this projectile. */
	UFUNCTION(BlueprintCallable, Category="Gravity Well")
	void DeactivateBlackHole();

	/** Returns whether the projectile currently has any gravity well active. */
	UFUNCTION(BlueprintPure, Category="Gravity Well")
	bool IsBlackHoleActive() const { return bBlackHoleActive; }

	/** Returns true if the active well is currently a white hole. */
	UFUNCTION(BlueprintPure, Category="Gravity Well")
	bool IsWhiteHoleActive() const { return bBlackHoleActive && bIsWhiteHole; }

	/** Broadcast when the projectile successfully activates the gravity well. */
	FGravityWellProjectileActivatedSignature OnBlackHoleActivated;

	/** Broadcast when the projectile removes the gravity well (or is destroyed). */
	FGravityWellProjectileDeactivatedSignature OnBlackHoleDeactivated;

protected:
	/** Class of gravity well actor to spawn on first activation (black hole). */
	UPROPERTY(EditAnywhere, Category="Gravity Well")
	TSubclassOf<AGravityWellActor> GravityWellClass;

	/** Class of gravity well actor to spawn when transforming into a white hole. */
	UPROPERTY(EditAnywhere, Category="Gravity Well")
	TSubclassOf<AGravityWellActor> WhiteHoleClass;

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

	/** Tracks whether the projectile currently has any gravity well active. */
	bool bBlackHoleActive = false;

	/** True if the current well is a white hole rather than a black hole. */
	bool bIsWhiteHole = false;

	/** Pointer to the spawned gravity well actor, if any. */
	TWeakObjectPtr<AGravityWellActor> ActiveWell;
};
