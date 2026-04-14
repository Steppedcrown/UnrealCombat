// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_Expel.generated.h"

class UCombatMoveData;
class UCombatHitDetectionComponent;
class UGameplayEffect;

/**
 *  Expel gameplay ability.
 *
 *  Activation flow:
 *    1. CanActivateAbility() checks that the caster has >= NodeCost Nodes
 *    2. Apply State.Combat.Attacking tag (via ActivationOwnedTags)
 *    3. Apply GE_ConsumeNode to self
 *    4. Look up DA_Expel from the move registry and play its AnimationMontage
 *    5. Listen for ActiveFramesBeginTag / ActiveFramesEndTag events to window hit detection
 *    6. On each hit:
 *         - Apply DamageEffectClass to target
 *         - Apply TempNodeEffectClass to target
 *    7. On montage end / cancel → EndAbility
 *
 *  Create Blueprint child BP_GA_Expel in Content/Combat/Abilities/.
 */
UCLASS()
class UGA_Expel : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Expel();

protected:

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
									const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayTagContainer* SourceTags = nullptr,
									const FGameplayTagContainer* TargetTags = nullptr,
									FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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
	//  Configuration — set in Blueprint child BP_GA_Expel
	// -----------------------------------------------------------------------

	/** Gameplay tag used to look up this move's data in the character's UCombatMoveRegistry */
	UPROPERTY(EditDefaultsOnly, Category="Expel")
	FGameplayTag MoveTag;

	/** Gameplay event tag fired by ANS_ActiveFrames when active frames begin */
	UPROPERTY(EditDefaultsOnly, Category="Expel")
	FGameplayTag ActiveFramesBeginTag;

	/** Gameplay event tag fired by ANS_ActiveFrames when active frames end */
	UPROPERTY(EditDefaultsOnly, Category="Expel")
	FGameplayTag ActiveFramesEndTag;

	/** Instant GE applied to self on activation to consume Nodes (e.g. GE_ConsumeNode) */
	UPROPERTY(EditDefaultsOnly, Category="Expel|Effects")
	TSubclassOf<UGameplayEffect> ConsumeNodeEffectClass;

	/** Instant GE applied to the target on hit (e.g. GE_DamageHealth) */
	UPROPERTY(EditDefaultsOnly, Category="Expel|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Duration GE applied to the target on hit (e.g. GE_ApplyTempNode) */
	UPROPERTY(EditDefaultsOnly, Category="Expel|Effects")
	TSubclassOf<UGameplayEffect> TempNodeEffectClass;

private:

	// -----------------------------------------------------------------------
	//  Internal helpers
	// -----------------------------------------------------------------------

	/** Fetches this ability's UCombatMoveData from the owning character's move registry */
	UCombatMoveData* GetMoveData() const;

	/** Applies ConsumeNodeEffectClass to self, scaled by NodeCost from move data */
	void ConsumeNodes();

	/** Applies DamageEffectClass and TempNodeEffectClass to HitActor */
	void ApplyHitEffects(AActor* HitActor);

	// -----------------------------------------------------------------------
	//  Montage task callbacks
	// -----------------------------------------------------------------------

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

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

	/** Bound to UCombatHitDetectionComponent::OnHit */
	void OnHitDetected(const FHitResult& HitResult);

	/** Cached pointer to the owner's hit detection component */
	UPROPERTY()
	TObjectPtr<UCombatHitDetectionComponent> HitDetectionComponent;
};
