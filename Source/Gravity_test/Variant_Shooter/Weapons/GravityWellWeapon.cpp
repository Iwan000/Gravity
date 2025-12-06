#include "GravityWellWeapon.h"

#include "GravityWellProjectile.h"

AGravityWellWeapon::AGravityWellWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	BlackHoleProjectileClass = AGravityWellProjectile::StaticClass();
}

void AGravityWellWeapon::BeginPlay()
{
	Super::BeginPlay();

	ProjectileClass = BlackHoleProjectileClass;
}

void AGravityWellWeapon::StartFiring()
{
	HandlePrimaryFire();
}

void AGravityWellWeapon::StopFiring()
{
	Super::StopFiring();
}

void AGravityWellWeapon::StartSecondaryFire()
{
	HandleSecondaryFire();
}

void AGravityWellWeapon::StopSecondaryFire()
{
}

void AGravityWellWeapon::HandlePrimaryFire()
{
	const bool bHadFlyingProjectile = FlyingProjectile.IsValid();

	if (AGravityWellProjectile* Existing = FlyingProjectile.Get())
	{
		Existing->Destroy();
		FlyingProjectile.Reset();
	}

	if (!BlackHoleProjectileClass)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (!bHadFlyingProjectile && CurrentTime - LastShotTime < RefireRate)
	{
		return;
	}

	if (AGravityWellProjectile* NewProjectile = SpawnGravityProjectile())
	{
		FlyingProjectile = NewProjectile;
		LastShotTime = CurrentTime;
		TimeOfLastShot = CurrentTime;

		if (PawnOwner)
		{
			MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, ShotNoiseTag);
		}
	}
}

void AGravityWellWeapon::HandleSecondaryFire()
{
	if (AGravityWellProjectile* Flying = FlyingProjectile.Get())
	{
		ActivateFlyingProjectile();
		return;
	}

	if (AGravityWellProjectile* ActiveHole = ActiveHoleProjectile.Get())
	{
		if (!ActiveHole->IsBlackHoleActive())
		{
			ActiveHoleProjectile.Reset();
			return;
		}

		if (!ActiveHole->IsWhiteHoleActive())
		{
			ActiveHole->TransformToWhiteHole();
		}
		else
		{
			ActiveHole->DeactivateBlackHole();
		}
	}
}

void AGravityWellWeapon::ActivateFlyingProjectile()
{
	if (AGravityWellProjectile* Flying = FlyingProjectile.Get())
	{
		Flying->ActivateBlackHole();
		FlyingProjectile.Reset();
	}
}

AGravityWellProjectile* AGravityWellWeapon::SpawnGravityProjectile()
{
	if (!BlackHoleProjectileClass || !WeaponOwner)
	{
		return nullptr;
	}

	const FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
	AShooterProjectile* SpawnedProjectile = SpawnProjectileOfClass(TargetLocation, BlackHoleProjectileClass);
	if (AGravityWellProjectile* GravityProjectile = Cast<AGravityWellProjectile>(SpawnedProjectile))
	{
		BindProjectileDelegates(GravityProjectile);
		return GravityProjectile;
	}

	return nullptr;
}

void AGravityWellWeapon::BindProjectileDelegates(AGravityWellProjectile* Projectile)
{
	if (!Projectile)
	{
		return;
	}

	Projectile->OnBlackHoleActivated.AddUObject(this, &AGravityWellWeapon::HandleProjectileActivated);
	Projectile->OnBlackHoleDeactivated.AddUObject(this, &AGravityWellWeapon::HandleProjectileDeactivated);
	Projectile->OnDestroyed.AddDynamic(this, &AGravityWellWeapon::HandleProjectileDestroyed);
}

void AGravityWellWeapon::HandleProjectileDestroyed(AActor* DestroyedActor)
{
	if (AGravityWellProjectile* Projectile = Cast<AGravityWellProjectile>(DestroyedActor))
	{
		if (FlyingProjectile.Get() == Projectile)
		{
			FlyingProjectile.Reset();
		}

		if (ActiveHoleProjectile.Get() == Projectile)
		{
			ActiveHoleProjectile.Reset();
		}
	}
}

void AGravityWellWeapon::HandleProjectileActivated(AGravityWellProjectile* Projectile)
{
	if (!Projectile)
	{
		return;
	}

	if (AGravityWellProjectile* ExistingActive = ActiveHoleProjectile.Get())
	{
		if (ExistingActive != Projectile && ExistingActive->IsBlackHoleActive())
		{
			ExistingActive->DeactivateBlackHole();
		}
	}

	ActiveHoleProjectile = Projectile;

	if (FlyingProjectile.Get() == Projectile)
	{
		FlyingProjectile.Reset();
	}
}

void AGravityWellWeapon::HandleProjectileDeactivated(AGravityWellProjectile* Projectile)
{
	if (ActiveHoleProjectile.Get() == Projectile)
	{
		ActiveHoleProjectile.Reset();
	}
}
