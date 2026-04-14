// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_BasicAttack.generated.h"

class UCombatMoveData;
class UCombatHitDetectionComponent;
class UGameplayEffect;

/**
 *  Basic attack gameplay ability.
 *
 *  Activation flow:
 *    1. Apply State.Combat.Attacking tag (via ActivationOwnedTags)
 *    2. Look up DA_BasicAttack from the character's move registry
 *    3. Play the move's AnimationMontage via UAbilityTask_PlayMontageAndWait
 *    4. Listen for ActiveFramesBeginTag event (sent by the animation notify state) → StartTrace()
 *    5. Listen for ActiveFramesEndTag event → StopTrace()
 *    6. On each hit broadcast by UCombatHitDetectionComponent:
 *         - Apply DamageEffectClass to target
 *         - Apply NodeRestoreEffectClass to self
 *    7. On montage end / cancel → EndAbility (tag auto-removed)
 *
 *  Create Blueprint child BP_GA_BasicAttack to assign MoveTag, event tags, and GE classes.
 */
UCLASS()
class UGA_BasicAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_BasicAttack();

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								 const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility,
							bool bWasCancelled) override;

	// -----------------------------------------------------------------------
	//  Configuration — set in Blueprint child BP_GA_BasicAttack
	// -----------------------------------------------------------------------

	/** Gameplay tag used to look up this move's data in the character's UCombatMoveRegistry */
	UPROPERTY(EditDefaultsOnly, Category="BasicAttack")
	FGameplayTag MoveTag;

	/**
	 *  Gameplay event tag fired by the animation notify state when active frames begin.
	 *  The AnimNotify should call UAbilitySystemBlueprintLibrary::SendGameplayEventToActor
	 *  with this tag on the owning character.
	 */
	UPROPERTY(EditDefaultsOnly, Category="BasicAttack")
	FGameplayTag ActiveFramesBeginTag;

	/**
	 *  Gameplay event tag fired when active frames end.
	 *  Mirrors ActiveFramesBeginTag — same AnimNotify state, notify end callback.
	 */
	UPROPERTY(EditDefaultsOnly, Category="BasicAttack")
	FGameplayTag ActiveFramesEndTag;

	/** Instant GE applied to the target on each hit (e.g. GE_DamageHealth) */
	UPROPERTY(EditDefaultsOnly, Category="BasicAttack|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Instant GE applied to self on each hit (e.g. GE_RestoreNode) */
	UPROPERTY(EditDefaultsOnly, Category="BasicAttack|Effects")
	TSubclassOf<UGameplayEffect> NodeRestoreEffectClass;

private:

	// -----------------------------------------------------------------------
	//  Internal helpers
	// -----------------------------------------------------------------------

	/** Fetches this ability's UCombatMoveData from the owning character's move registry */
	UCombatMoveData* GetMoveData() const;

	/** Applies DamageEffectClass to HitActor and NodeRestoreEffectClass to self */
	void ApplyHitEffects(AActor* HitActor);

	// -----------------------------------------------------------------------
	//  Montage task callbacks
	// -----------------------------------------------------------------------

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageBlendOut(bool bInterrupted);

	// -----------------------------------------------------------------------
	//  Gameplay event task callbacks
	// -----------------------------------------------------------------------

	UFUNCTION()
	void OnActiveFramesBegin(FGameplayEventData Payload);

	UFUNCTION()
	void OnActiveFramesEnd(FGameplayEventData Payload);

	// -----------------------------------------------------------------------
	//  Hit detection
	// -----------------------------------------------------------------------

	/**
	 *  Bound to UCombatHitDetectionComponent::OnHit.
	 *  Called once per unique actor hit during an active sweep.
	 */
	void OnHitDetected(const FHitResult& HitResult);

	/** Cached pointer to the owner's hit detection component */
	UPROPERTY()
	TObjectPtr<UCombatHitDetectionComponent> HitDetectionComponent;
};
