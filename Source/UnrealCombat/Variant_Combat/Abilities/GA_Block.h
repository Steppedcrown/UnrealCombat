// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_Block.generated.h"

class UCombatMoveData;
class UGameplayEffect;

/**
 *  Block gameplay ability.
 *
 *  Activation flow:
 *    1. Apply State.Combat.Blocking tag (via ActivationOwnedTags)
 *    2. Record activation timestamp for perfect block detection
 *    3. Play the block montage from the move registry (if set)
 *    4. Start a timer for BlockWindowDuration (~1 second)
 *    5. While blocking: UCombatAttributeSet::PostGameplayEffectExecute detects the tag
 *       and negates incoming GE_DamageHealth, then fires a BlockHitEventTag gameplay event
 *    6. If BlockHitEventTag arrives within PerfectBlockWindowDuration of activation:
 *         - Apply NodeRestoreEffectClass to self
 *         - Apply KnockbackEffectClass to all enemies within PerfectBlockRadius
 *    7. When BlockWindowDuration expires: EndAbility (tag auto-removed), apply cooldown
 *
 *  Create Blueprint child BP_GA_Block in Content/Combat/Abilities/.
 */
UCLASS()
class UGA_Block : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Block();

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
	//  Configuration — set in Blueprint child BP_GA_Block
	// -----------------------------------------------------------------------

	/** Gameplay tag used to look up this move's data in the character's UCombatMoveRegistry */
	UPROPERTY(EditDefaultsOnly, Category="Block")
	FGameplayTag MoveTag;

	/**
	 *  Duration of the block window in seconds.
	 *  After this time the ability ends and the cooldown begins.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Block", meta=(ClampMin=0.1f))
	float BlockWindowDuration = 1.0f;

	/**
	 *  Time window (in seconds) after activation in which a blocked hit counts as a perfect block.
	 *  Corresponds roughly to ~9 frames at 60 fps.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Block", meta=(ClampMin=0.0f))
	float PerfectBlockWindowDuration = 0.15f;

	/**
	 *  Sphere radius used to find enemies that receive knockback on a perfect block.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Block", meta=(ClampMin=0.0f, Units="cm"))
	float PerfectBlockRadius = 300.0f;

	/**
	 *  Gameplay event tag fired by UCombatAttributeSet when an incoming hit is absorbed
	 *  while State.Combat.Blocking is active.  The ability listens for this event to
	 *  detect whether a perfect block occurred.
	 */
	UPROPERTY(EditDefaultsOnly, Category="Block")
	FGameplayTag BlockHitEventTag;

	/** Instant GE applied to self on a perfect block (e.g. GE_RestoreNode) */
	UPROPERTY(EditDefaultsOnly, Category="Block|Effects")
	TSubclassOf<UGameplayEffect> NodeRestoreEffectClass;

	/** Instant GE applied to each enemy in range on a perfect block (e.g. GE_Knockback) */
	UPROPERTY(EditDefaultsOnly, Category="Block|Effects")
	TSubclassOf<UGameplayEffect> KnockbackEffectClass;

private:

	// -----------------------------------------------------------------------
	//  Internal helpers
	// -----------------------------------------------------------------------

	/** Fetches this ability's UCombatMoveData from the owning character's move registry */
	UCombatMoveData* GetMoveData() const;

	/** Called when the block window timer expires */
	UFUNCTION()
	void OnBlockWindowExpired();

	/** Triggered by the BlockHitEventTag gameplay event when a hit is absorbed by the block */
	UFUNCTION()
	void OnDamageBlocked(FGameplayEventData Payload);

	/**
	 *  Applies perfect block consequences:
	 *    - GE_RestoreNode to self
	 *    - GE_Knockback to all enemies within PerfectBlockRadius
	 *  @param Attacker  The actor whose hit triggered the perfect block (may be nullptr)
	 */
	void TriggerPerfectBlock(AActor* Attacker);

	/** Applies KnockbackEffectClass to a single target */
	void ApplyKnockbackToTarget(AActor* Target);

	// -----------------------------------------------------------------------
	//  Montage task callbacks
	// -----------------------------------------------------------------------

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	// -----------------------------------------------------------------------
	//  State
	// -----------------------------------------------------------------------

	/** World time (seconds) at the moment this ability activated — used for perfect block timing */
	float ActivationTime = 0.0f;

	/** Timer that ends the block window */
	FTimerHandle BlockWindowTimerHandle;
};
