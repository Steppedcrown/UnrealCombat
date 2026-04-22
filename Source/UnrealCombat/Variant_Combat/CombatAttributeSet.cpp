// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UCombatAttributeSet::UCombatAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitNodes(4.0f);
	InitMaxNodes(4.0f);
	InitTempNodes(0.0f);
}

void UCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PostGEExecute: owner=%s attribute=%s magnitude=%.2f"),
		*GetNameSafe(GetOwningActor()),
		*Data.EvaluatedData.Attribute.GetName(),
		Data.EvaluatedData.Magnitude);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		UE_LOG(LogTemp, Warning, TEXT("  Health changed: %.1f / %.1f"), GetHealth(), GetMaxHealth());
		// If this is incoming damage (negative modifier) and the owner is blocking,
		// negate the change and fire an event so UGA_Block can detect a perfect block.
		const FGameplayTag BlockingTag = FGameplayTag::RequestGameplayTag(FName("State.Combat.Blocking"));
		if (Data.EvaluatedData.Magnitude < 0.0f && ASC->HasMatchingGameplayTag(BlockingTag))
		{
			// Restore health to its pre-effect value
			const float RestoredHealth = FMath::Clamp(GetHealth() - Data.EvaluatedData.Magnitude, 0.0f, GetMaxHealth());
			SetHealth(RestoredHealth);

			// Notify UGA_Block that a hit was absorbed so it can check the perfect block window
			const FGameplayTag BlockHitTag = FGameplayTag::RequestGameplayTag(FName("Event.Block.Hit"), /*bErrorIfNotFound=*/false);
			if (BlockHitTag.IsValid())
			{
				FGameplayEventData BlockedPayload;
				BlockedPayload.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
				ASC->HandleGameplayEvent(BlockHitTag, &BlockedPayload);
			}
			return;
		}

		// clamp health
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		// reduce MaxNodes by 1 for each 20% health threshold crossed below full health
		// e.g. 79% health = -1, 59% = -2, 39% = -3, 19% = -4
		const float HealthPct = GetMaxHealth() > 0.0f ? GetHealth() / GetMaxHealth() : 0.0f;
		const int32 Penalty = FMath::FloorToInt((1.0f - HealthPct) / NodePenaltyThreshold);
		const float NewMaxNodes = FMath::Max(0.0f, BaseMaxNodes - static_cast<float>(Penalty));
		SetMaxNodes(NewMaxNodes);

		// clamp current Nodes to the new maximum
		SetNodes(FMath::Clamp(GetNodes(), 0.0f, GetMaxNodes()));

		// apply dead tag when health hits zero
		if (GetHealth() <= 0.0f)
		{
			ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Status.Dead")));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetNodesAttribute())
	{
		// clamp nodes
		SetNodes(FMath::Clamp(GetNodes(), 0.0f, GetMaxNodes()));

		// manage vulnerable tag based on whether any nodes remain
		const FGameplayTag VulnerableTag = FGameplayTag::RequestGameplayTag(FName("State.Status.Vulnerable"));
		if (GetNodes() <= 0.0f)
		{
			if (!ASC->HasMatchingGameplayTag(VulnerableTag))
			{
				ASC->AddLooseGameplayTag(VulnerableTag);
			}
		}
		else
		{
			if (ASC->HasMatchingGameplayTag(VulnerableTag))
			{
				ASC->RemoveLooseGameplayTag(VulnerableTag);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxNodesAttribute())
	{
		// clamp MaxNodes to a non-negative value and pull down current Nodes if needed
		SetMaxNodes(FMath::Max(0.0f, GetMaxNodes()));
		SetNodes(FMath::Clamp(GetNodes(), 0.0f, GetMaxNodes()));
	}
	else if (Data.EvaluatedData.Attribute == GetTempNodesAttribute())
	{
		// TempNodes have no upper cap but cannot go negative
		SetTempNodes(FMath::Max(0.0f, GetTempNodes()));
	}
}
