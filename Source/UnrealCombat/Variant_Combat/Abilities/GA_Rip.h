// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_Rip.generated.h"

class UCombatMoveData;
class UCombatHitDetectionComponent;
class UGameplayEffect;

/**
 *  Rip gameplay ability.
 *
 *  Activation flow:
 *    1. CanActivateAbility() checks NodeCost
 *    2. Apply State.Combat.Attacking tag (via ActivationOwnedTags)
 *    3. Consume NodeCost Nodes from self
 *    4. Play DA_Rip's AnimationMontage; window hit detection with ANS_ActiveFrames
 *    5. On hit — branch on target's State.Status.Vulnerable tag:
 *         NOT Vulnerable: apply GE_DamageHealth (minor) + steal all Nodes from target
 *         Vulnerable:     apply GE_Kill to target
 *    6. Montage end → EndAbility
 *
 *  Create Blueprint child BP_GA_Rip in Content/Combat/Abilities/.
 */
UCLASS()
class UGA_Rip : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_Rip();

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
	//  Configuration — set in Blueprint child BP_GA_Rip
	// -----------------------------------------------------------------------

	/** Gameplay tag used to look up this move's data in the character's UCombatMoveRegistry */
	UPROPERTY(EditDefaultsOnly, Category="Rip")
	FGameplayTag MoveTag;

	/** Gameplay event tag fired by ANS_ActiveFrames when active frames begin */
	UPROPERTY(EditDefaultsOnly, Category="Rip")
	FGameplayTag ActiveFramesBeginTag;

	/** Gameplay event tag fired by ANS_ActiveFrames when active frames end */
	UPROPERTY(EditDefaultsOnly, Category="Rip")
	FGameplayTag ActiveFramesEndTag;

	/** Instant GE applied to self on activation to consume Nodes (e.g. GE_ConsumeNode) */
	UPROPERTY(EditDefaultsOnly, Category="Rip|Effects")
	TSubclassOf<UGameplayEffect> ConsumeNodeEffectClass;

	/** Minor damage GE applied to target when NOT Vulnerable (e.g. GE_DamageHealth) */
	UPROPERTY(EditDefaultsOnly, Category="Rip|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** GE applied to target once per stolen Node when NOT Vulnerable (e.g. GE_ConsumeNode) */
	UPROPERTY(EditDefaultsOnly, Category="Rip|Effects")
	TSubclassOf<UGameplayEffect> StealNodeFromTargetEffectClass;

	/** GE applied to self once per stolen Node when NOT Vulnerable (e.g. GE_RestoreNode) */
	UPROPERTY(EditDefaultsOnly, Category="Rip|Effects")
	TSubclassOf<UGameplayEffect> RestoreNodeEffectClass;

	/** GE that sets target Health to 0 when Vulnerable (e.g. GE_Kill) */
	UPROPERTY(EditDefaultsOnly, Category="Rip|Effects")
	TSubclassOf<UGameplayEffect> KillEffectClass;

private:

	UCombatMoveData* GetMoveData() const;
	void ConsumeNodes();
	void ApplyHitEffects(AActor* HitActor);
	/** Steals up to StealCap Nodes from target (0 = steal all) */
	void StealNodes(AActor* TargetActor, UAbilitySystemComponent* TargetASC, int32 StealCap);

	UFUNCTION() void OnMontageCompleted();
	UFUNCTION() void OnMontageCancelled();
	UFUNCTION() void OnActiveFramesBegin(FGameplayEventData Payload);
	UFUNCTION() void OnActiveFramesEnd(FGameplayEventData Payload);

	void OnHitDetected(const FHitResult& HitResult);

	UPROPERTY()
	TObjectPtr<UCombatHitDetectionComponent> HitDetectionComponent;
};
