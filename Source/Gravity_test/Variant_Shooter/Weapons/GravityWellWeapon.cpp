#include "GravityWellWeapon.h"

#include "GravityWellProjectile.h"
#include "WhiteHoleProjectile.h"

AGravityWellWeapon::AGravityWellWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	BlackHoleProjectileClass = AGravityWellProjectile::StaticClass();
	WhiteHoleProjectileClass = AWhiteHoleProjectile::StaticClass();
}

void AGravityWellWeapon::BeginPlay()
{
	Super::BeginPlay();

	// Ensure the base projectile class defaults to the black hole variant for legacy logic.
	if (BlackHoleProjectileClass)
	{
		ProjectileClass = BlackHoleProjectileClass;
	}
}

void AGravityWellWeapon::StartFiring()
{
	HandleTriggerPressed(true);
}

void AGravityWellWeapon::StopFiring()
{
	Super::StopFiring();
}

void AGravityWellWeapon::StartSecondaryFire()
{
	HandleTriggerPressed(false);
}

void AGravityWellWeapon::StopSecondaryFire()
{
}

void AGravityWellWeapon::HandleTriggerPressed(bool bIsBlackHole)
{
	PromotePendingIfActivated(bIsBlackHole);

	TWeakObjectPtr<AGravityWellProjectile>& ActiveRef = bIsBlackHole ? ActiveBlackProjectile : ActiveWhiteProjectile;
	TWeakObjectPtr<AGravityWellProjectile>& PendingRef = bIsBlackHole ? PendingBlackProjectile : PendingWhiteProjectile;
	const TSubclassOf<AGravityWellProjectile> ProjectileClassToUse = bIsBlackHole ? BlackHoleProjectileClass : WhiteHoleProjectileClass;

	if (AGravityWellProjectile* Active = ActiveRef.Get())
	{
		Active->DeactivateBlackHole();
		return;
	}

	if (AGravityWellProjectile* Pending = PendingRef.Get())
	{
		Pending->ActivateBlackHole();
		ActiveRef = Pending;
		PendingRef.Reset();
		return;
	}

	if (!ProjectileClassToUse)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	float& LastShotRef = bIsBlackHole ? LastBlackShotTime : LastWhiteShotTime;
	if (CurrentTime - LastShotRef < RefireRate)
	{
		return;
	}

	if (AGravityWellProjectile* NewProjectile = SpawnGravityProjectile(ProjectileClassToUse, bIsBlackHole))
	{
		PendingRef = NewProjectile;
		ActiveRef.Reset();

		TimeOfLastShot = CurrentTime;
		LastShotRef = CurrentTime;

		if (PawnOwner)
		{
			MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, ShotNoiseTag);
		}
	}
}

void AGravityWellWeapon::PromotePendingIfActivated(bool bIsBlackHole)
{
	TWeakObjectPtr<AGravityWellProjectile>& ActiveRef = bIsBlackHole ? ActiveBlackProjectile : ActiveWhiteProjectile;
	TWeakObjectPtr<AGravityWellProjectile>& PendingRef = bIsBlackHole ? PendingBlackProjectile : PendingWhiteProjectile;

	if (ActiveRef.IsValid())
	{
		return;
	}

	if (AGravityWellProjectile* Pending = PendingRef.Get())
	{
		if (Pending->IsBlackHoleActive())
		{
			ActiveRef = Pending;
			PendingRef.Reset();
		}
	}
}

AGravityWellProjectile* AGravityWellWeapon::SpawnGravityProjectile(TSubclassOf<AGravityWellProjectile> ProjectileClassToUse, bool bIsBlackHole)
{
	if (!ProjectileClassToUse || !WeaponOwner)
	{
		return nullptr;
	}

	const FVector TargetLocation = WeaponOwner->GetWeaponTargetLocation();
	AShooterProjectile* SpawnedProjectile = SpawnProjectileOfClass(TargetLocation, ProjectileClassToUse);
	if (AGravityWellProjectile* GravityProjectile = Cast<AGravityWellProjectile>(SpawnedProjectile))
	{
		BindProjectileDelegates(GravityProjectile, bIsBlackHole);
		return GravityProjectile;
	}

	return nullptr;
}

void AGravityWellWeapon::BindProjectileDelegates(AGravityWellProjectile* Projectile, bool bIsBlackHole)
{
	if (!Projectile)
	{
		return;
	}

	if (bIsBlackHole)
	{
		Projectile->OnBlackHoleActivated.AddUObject(this, &AGravityWellWeapon::HandleBlackProjectileActivated);
		Projectile->OnBlackHoleDeactivated.AddUObject(this, &AGravityWellWeapon::HandleBlackProjectileDeactivated);
		Projectile->OnDestroyed.AddDynamic(this, &AGravityWellWeapon::OnBlackProjectileDestroyed);
	}
	else
	{
		Projectile->OnBlackHoleActivated.AddUObject(this, &AGravityWellWeapon::HandleWhiteProjectileActivated);
		Projectile->OnBlackHoleDeactivated.AddUObject(this, &AGravityWellWeapon::HandleWhiteProjectileDeactivated);
		Projectile->OnDestroyed.AddDynamic(this, &AGravityWellWeapon::OnWhiteProjectileDestroyed);
	}
}

void AGravityWellWeapon::ClearProjectilePointers(AGravityWellProjectile* Projectile, bool bIsBlackHole)
{
	if (!Projectile)
	{
		return;
	}

	if (bIsBlackHole)
	{
		if (PendingBlackProjectile.Get() == Projectile)
		{
			PendingBlackProjectile.Reset();
		}

		if (ActiveBlackProjectile.Get() == Projectile)
		{
			ActiveBlackProjectile.Reset();
		}
	}
	else
	{
		if (PendingWhiteProjectile.Get() == Projectile)
		{
			PendingWhiteProjectile.Reset();
		}

		if (ActiveWhiteProjectile.Get() == Projectile)
		{
			ActiveWhiteProjectile.Reset();
		}
	}
}

void AGravityWellWeapon::OnBlackProjectileDestroyed(AActor* DestroyedActor)
{
	if (AGravityWellProjectile* Projectile = Cast<AGravityWellProjectile>(DestroyedActor))
	{
		ClearProjectilePointers(Projectile, true);
	}
}

void AGravityWellWeapon::OnWhiteProjectileDestroyed(AActor* DestroyedActor)
{
	if (AGravityWellProjectile* Projectile = Cast<AGravityWellProjectile>(DestroyedActor))
	{
		ClearProjectilePointers(Projectile, false);
	}
}

void AGravityWellWeapon::HandleBlackProjectileActivated(AGravityWellProjectile* Projectile)
{
	ActiveBlackProjectile = Projectile;
	PendingBlackProjectile.Reset();
}

void AGravityWellWeapon::HandleBlackProjectileDeactivated(AGravityWellProjectile* Projectile)
{
	ClearProjectilePointers(Projectile, true);
}

void AGravityWellWeapon::HandleWhiteProjectileActivated(AGravityWellProjectile* Projectile)
{
	ActiveWhiteProjectile = Projectile;
	PendingWhiteProjectile.Reset();
}

void AGravityWellWeapon::HandleWhiteProjectileDeactivated(AGravityWellProjectile* Projectile)
{
	ClearProjectilePointers(Projectile, false);
}
