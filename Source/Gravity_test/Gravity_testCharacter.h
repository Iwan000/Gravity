// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Gravity_testCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class AGravity_testCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** First person pistol mesh (only visible to owning player) */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetFirstPersonPistolMesh, Category="Weapons", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonPistolMesh;

	/** Third person pistol mesh (hidden from owning player) */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetThirdPersonPistolMesh, Category="Weapons", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonPistolMesh;

	/** Socket used to attach the first person pistol */
	UPROPERTY(EditDefaultsOnly, Category="Weapons")
	FName FirstPersonPistolSocket = FName("GripPoint");

	/** Socket used to attach the third person pistol */
	UPROPERTY(EditDefaultsOnly, Category="Weapons")
	FName ThirdPersonPistolSocket = FName("hand_r");

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
	AGravity_testCharacter();

protected:

	/** Handle component setup that depends on loaded assets */
	virtual void BeginPlay() override;

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Returns the first person pistol mesh */
	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetFirstPersonPistolMesh() const { return FirstPersonPistolMesh; }

	/** Returns the third person pistol mesh */
	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetThirdPersonPistolMesh() const { return ThirdPersonPistolMesh; }

	/** Returns the socket name used for the first person pistol */
	UFUNCTION(BlueprintPure, Category="Weapons")
	FName GetFirstPersonPistolSocketName() const { return FirstPersonPistolSocket; }

	/** Returns the socket name used for the third person pistol */
	UFUNCTION(BlueprintPure, Category="Weapons")
	FName GetThirdPersonPistolSocketName() const { return ThirdPersonPistolSocket; }

};
