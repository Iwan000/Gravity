// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gravity_testCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"
#include "Gravity_test.h"

AGravity_testCharacter::AGravity_testCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// First person pistol mesh (only visible to the owning player)
	FirstPersonPistolMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Pistol"));
	FirstPersonPistolMesh->SetupAttachment(FirstPersonMesh);
	FirstPersonPistolMesh->SetOnlyOwnerSee(true);
	FirstPersonPistolMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	FirstPersonPistolMesh->SetCanEverAffectNavigation(false);
	FirstPersonPistolMesh->bCastDynamicShadow = false;
	FirstPersonPistolMesh->CastShadow = false;

	// Third person pistol mesh (hidden from the owning player but visible to others)
	ThirdPersonPistolMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Third Person Pistol"));
	ThirdPersonPistolMesh->SetupAttachment(GetMesh());
	ThirdPersonPistolMesh->SetOwnerNoSee(true);
	ThirdPersonPistolMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	ThirdPersonPistolMesh->SetCanEverAffectNavigation(false);
	ThirdPersonPistolMesh->bCastDynamicShadow = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> PistolMeshFinder(TEXT("/Game/Weapons/Pistol/Meshes/SK_Pistol.SK_Pistol"));
	if (PistolMeshFinder.Succeeded())
	{
		FirstPersonPistolMesh->SetSkeletalMesh(PistolMeshFinder.Object);
		ThirdPersonPistolMesh->SetSkeletalMesh(PistolMeshFinder.Object);
	}
	else
	{
		UE_LOG(LogGravity_test, Warning, TEXT("First person pistol mesh asset not found. Update the path in AGravity_testCharacter."));
	}

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void AGravity_testCharacter::BeginPlay()
{
	Super::BeginPlay();

	const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);

	if (FirstPersonMesh && FirstPersonPistolMesh)
	{
		if (!FirstPersonPistolSocket.IsNone() && FirstPersonMesh->DoesSocketExist(FirstPersonPistolSocket))
		{
			FirstPersonPistolMesh->AttachToComponent(FirstPersonMesh, AttachRules, FirstPersonPistolSocket);
		}
		else
		{
			FirstPersonPistolMesh->AttachToComponent(FirstPersonMesh, AttachRules);
			if (!FirstPersonPistolSocket.IsNone())
			{
				UE_LOG(LogGravity_test, Warning, TEXT("First person pistol socket '%s' not found on %s."), *FirstPersonPistolSocket.ToString(), *GetNameSafe(FirstPersonMesh));
			}
		}
	}

	if (USkeletalMeshComponent* ThirdPersonMesh = GetMesh())
	{
		if (ThirdPersonPistolMesh)
		{
			if (!ThirdPersonPistolSocket.IsNone() && ThirdPersonMesh->DoesSocketExist(ThirdPersonPistolSocket))
			{
				ThirdPersonPistolMesh->AttachToComponent(ThirdPersonMesh, AttachRules, ThirdPersonPistolSocket);
			}
			else
			{
				ThirdPersonPistolMesh->AttachToComponent(ThirdPersonMesh, AttachRules);
				if (!ThirdPersonPistolSocket.IsNone())
				{
					UE_LOG(LogGravity_test, Warning, TEXT("Third person pistol socket '%s' not found on %s."), *ThirdPersonPistolSocket.ToString(), *GetNameSafe(ThirdPersonMesh));
				}
			}
		}
	}
}

void AGravity_testCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGravity_testCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGravity_testCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGravity_testCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGravity_testCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGravity_testCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogGravity_test, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AGravity_testCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AGravity_testCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AGravity_testCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AGravity_testCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AGravity_testCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AGravity_testCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
