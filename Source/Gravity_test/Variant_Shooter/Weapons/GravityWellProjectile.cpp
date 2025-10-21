#include "GravityWellProjectile.h"

#include "GravityWellActor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AGravityWellProjectile::AGravityWellProjectile()
{
	// Override default projectile behaviour so the bullet itself does no damage.
	HitDamage = 0.0f;
	PhysicsForce = 0.0f;
	bExplodeOnHit = false;
	DeferredDestructionTime = 0.0f;
	GravityWellClass = AGravityWellActor::StaticClass();
}

void AGravityWellProjectile::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (bBlackHoleActive)
	{
		OnBlackHoleDeactivated.Broadcast(this);
		BP_OnBlackHoleDeactivated();
		bBlackHoleActive = false;
	}

	DestroyGravityWell();

	Super::EndPlay(EndPlayReason);
}

void AGravityWellProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	// Once the well is active we ignore further hits.
	if (bBlackHoleActive)
	{
		return;
	}

	// Snap to the impact point before activation.
	SetActorLocation(HitLocation);

	// Generate the usual noise cue for AI systems.
	MakeNoise(NoiseLoudness, GetInstigator(), GetActorLocation(), NoiseRange, NoiseTag);

	// Allow blueprint side-effects to react to the collision.
	BP_OnProjectileHit(Hit);

	ActivateBlackHole();
}

void AGravityWellProjectile::ActivateBlackHole()
{
	if (bBlackHoleActive)
	{
		return;
	}

	bBlackHoleActive = true;
	bHit = true;

	// Stop any further movement or collision.
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	GetWorldTimerManager().ClearTimer(DestructionTimer);

	SpawnGravityWell();

	OnBlackHoleActivated.Broadcast(this);
	BP_OnBlackHoleActivated();
}

void AGravityWellProjectile::DeactivateBlackHole()
{
	if (!bBlackHoleActive)
	{
		Destroy();
		return;
	}

	bBlackHoleActive = false;

	OnBlackHoleDeactivated.Broadcast(this);
	BP_OnBlackHoleDeactivated();

	DestroyGravityWell();

	if (bDestroyProjectileWithWell)
	{
		Destroy();
	}
}

void AGravityWellProjectile::SpawnGravityWell()
{
	if (!ensure(GetWorld()))
	{
		return;
	}

	if (!GravityWellClass)
	{
		return;
	}

	const FTransform SpawnTransform = FTransform(GetActorRotation(), GetActorLocation() + WellSpawnOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AGravityWellActor* Well = GetWorld()->SpawnActor<AGravityWellActor>(GravityWellClass, SpawnTransform, SpawnParams))
	{
		ActiveWell = Well;
		Well->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		Well->OnDestroyed.AddDynamic(this, &AGravityWellProjectile::HandleWellDestroyed);
	}
}

void AGravityWellProjectile::DestroyGravityWell()
{
	if (AGravityWellActor* Well = ActiveWell.Get())
	{
		if (IsValid(Well))
		{
			Well->OnDestroyed.RemoveDynamic(this, &AGravityWellProjectile::HandleWellDestroyed);
			Well->Destroy();
		}

		ActiveWell.Reset();
	}
}

void AGravityWellProjectile::HandleWellDestroyed(AActor* DestroyedActor)
{
	if (ActiveWell.Get() != DestroyedActor)
	{
		return;
	}

	if (AGravityWellActor* Well = ActiveWell.Get())
	{
		if (IsValid(Well))
		{
			Well->OnDestroyed.RemoveDynamic(this, &AGravityWellProjectile::HandleWellDestroyed);
		}
	}

	ActiveWell.Reset();

	if (bBlackHoleActive)
	{
		bBlackHoleActive = false;
		OnBlackHoleDeactivated.Broadcast(this);
		BP_OnBlackHoleDeactivated();
	}
}
