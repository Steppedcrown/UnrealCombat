// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CombatCharacter.h"
#include "CombatPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;

/**
 *  Player-only extension of ACombatCharacter.
 *  Adds the camera boom, follow camera, and camera-side toggle — none of which enemies need.
 */
UCLASS(abstract)
class ACombatPlayerCharacter : public ACombatCharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	UCameraComponent* FollowCamera;

protected:

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleCameraAction;

	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin=0, ClampMax=1000, Units="cm"))
	float DefaultCameraDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin=0, ClampMax=1000, Units="cm"))
	float DeathCameraDistance = 400.0f;

public:

	ACombatPlayerCharacter();

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void HandleDeath() override;

protected:

	void ToggleCamera();

	UFUNCTION(BlueprintImplementableEvent, Category="Combat")
	void BP_ToggleCamera();
};
