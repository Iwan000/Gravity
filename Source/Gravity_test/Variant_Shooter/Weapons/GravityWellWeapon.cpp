#include "GravityWellWeapon.h"

#include "GravityWellProjectile.h"

AGravityWellWeapon::AGravityWellWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGravityWellWeapon::StartFiring()
{
	PromotePendingIfActivated();

	if (AGravityWellProjectile* Active = ActiveProjectile.Get())
	{
		Active->DeactivateBlackHole();
		return;
	}

	if (AGravityWellProjectile* Pending = PendingProjectile.Get())
	{
		if (!Pending->IsBlackHoleActive())
		{
			Pending->ActivateBlackHole();
		}

		ActiveProjectile = Pending;
		PendingProjectile.Reset();
		return;
	}

	Super::StartFiring();
}

void AGravityWellWeapon::StopFiring()
{
	Super::StopFiring();
}

void AGravityWellWeapon::FireProjectile(const FVector& TargetLocation)
{
	Super::FireProjectile(TargetLocation);

	if (AGravityWellProjectile* SpawnedProjectile = Cast<AGravityWellProjectile>(GetLastFiredProjectile()))
	{
		BindToProjectile(SpawnedProjectile);
		PendingProjectile = SpawnedProjectile;
		ActiveProjectile.Reset();
	}
	else
	{
		PendingProjectile.Reset();
	}
}

void AGravityWellWeapon::BindToProjectile(AGravityWellProjectile* Projectile)
{
	if (!Projectile)
	{
		return;
	}

	Projectile->OnBlackHoleActivated.AddUObject(this, &AGravityWellWeapon::HandleProjectileActivated);
	Projectile->OnBlackHoleDeactivated.AddUObject(this, &AGravityWellWeapon::HandleProjectileDeactivated);
	Projectile->OnDestroyed.AddDynamic(this, &AGravityWellWeapon::OnTrackedProjectileDestroyed);
}

void AGravityWellWeapon::PromotePendingIfActivated()
{
	if (ActiveProjectile.IsValid())
	{
		return;
	}

	if (AGravityWellProjectile* Pending = PendingProjectile.Get())
	{
		if (Pending->IsBlackHoleActive())
		{
			ActiveProjectile = Pending;
			PendingProjectile.Reset();
		}
	}
}

void AGravityWellWeapon::ClearProjectileReferences(AGravityWellProjectile* Projectile)
{
	if (!Projectile)
	{
		return;
	}

	if (IsValid(Projectile))
	{
		Projectile->OnBlackHoleActivated.RemoveAll(this);
		Projectile->OnBlackHoleDeactivated.RemoveAll(this);
		Projectile->OnDestroyed.RemoveDynamic(this, &AGravityWellWeapon::OnTrackedProjectileDestroyed);
	}

	if (PendingProjectile.Get() == Projectile)
	{
		PendingProjectile.Reset();
	}

	if (ActiveProjectile.Get() == Projectile)
	{
		ActiveProjectile.Reset();
	}
}

void AGravityWellWeapon::OnTrackedProjectileDestroyed(AActor* DestroyedActor)
{
	if (AGravityWellProjectile* Projectile = Cast<AGravityWellProjectile>(DestroyedActor))
	{
		ClearProjectileReferences(Projectile);
	}
}

void AGravityWellWeapon::HandleProjectileActivated(AGravityWellProjectile* Projectile)
{
	if (!Projectile)
	{
		return;
	}

	if (PendingProjectile.Get() == Projectile)
	{
		ActiveProjectile = Projectile;
		PendingProjectile.Reset();
	}
}

void AGravityWellWeapon::HandleProjectileDeactivated(AGravityWellProjectile* Projectile)
{
	ClearProjectileReferences(Projectile);
}
