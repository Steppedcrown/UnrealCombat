// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CombatAttributeSet.generated.h"

// Generates boilerplate getter/setter/initter for a gameplay attribute
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 *  Attribute set for combat characters.
 *  Manages Health, Nodes, TempNodes and their derived maximums.
 *  Applies State.Status.Vulnerable when Nodes reach 0 and State.Status.Dead when Health reaches 0.
 *  MaxNodes is reduced by 1 for each 20% health threshold crossed below full health.
 */
UCLASS()
class UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	UCombatAttributeSet();

	// --- Health ---

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxHealth)

	// --- Nodes ---

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Nodes")
	FGameplayAttributeData Nodes;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, Nodes)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Nodes")
	FGameplayAttributeData MaxNodes;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, MaxNodes)

	// --- TempNodes ---

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Nodes")
	FGameplayAttributeData TempNodes;
	ATTRIBUTE_ACCESSORS(UCombatAttributeSet, TempNodes)

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** The maximum number of Nodes at full health. Penalties are subtracted from this at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes|Nodes")
	float BaseMaxNodes = 4.0f;

	/** Health percentage interval at which MaxNodes is reduced by 1 (e.g. 0.2 = every 20%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes|Nodes", meta=(ClampMin=0.01, ClampMax=1.0))
	float NodePenaltyThreshold = 0.2f;
};
