// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LockOnComponent.generated.h"

class ACombatCharacter;
class USpringArmComponent;

/**
 *  Handles lock-on targeting for ACombatCharacter.
 *  On toggle: sphere-overlaps for nearby enemies within a forward cone and locks to the closest valid one.
 *  While locked: smoothly rotates the owner's spring arm to face the target each tick.
 *  Clears the target when toggled again or when the target dies.
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	ULockOnComponent();

	/** Maximum distance at which a target can be acquired */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lock-On", meta=(ClampMin=0, ClampMax=5000, Units="cm"))
	float LockOnRange = 1500.0f;

	/** Half-angle of the forward cone used to filter candidate targets (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lock-On", meta=(ClampMin=0, ClampMax=180))
	float LockOnAngle = 60.0f;

	/** Speed at which the spring arm rotates toward the locked target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lock-On", meta=(ClampMin=0, ClampMax=20))
	float LockOnInterpSpeed = 10.0f;

	/** True while a target is locked */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lock-On")
	bool bIsLocked = false;

	/** Draw debug shapes for lock-on acquisition each time ToggleLockOn is called */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lock-On|Debug")
	bool bDebugAcquire = false;

	/** Toggles lock-on on/off. Called from ACombatCharacter::LockOnPressed(). */
	void ToggleLockOn();

	/** Returns the currently locked target, or nullptr if not locked */
	FORCEINLINE ACombatCharacter* GetTarget() const { return TargetActor; }

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** The currently locked target */
	UPROPERTY()
	TObjectPtr<ACombatCharacter> TargetActor;

	/** Cached spring arm on the owner */
	UPROPERTY()
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Searches for and locks onto the best candidate target. Returns true if a target was found. */
	bool AcquireTarget();

	/** Clears the current lock-on target and restores free camera */
	void ClearTarget();

	/** Called when the locked target is destroyed */
	UFUNCTION()
	void OnTargetDestroyed(AActor* DestroyedActor);
};
