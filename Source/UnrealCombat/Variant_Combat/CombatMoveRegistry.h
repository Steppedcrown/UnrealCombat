// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CombatMoveRegistry.generated.h"

class UCombatMoveData;

/**
 *  Maps gameplay tags to their corresponding move data assets.
 *  Create one instance (DA_MoveRegistry) in Content/Combat/ and populate it in the editor.
 *  Reference it from ACombatCharacter so abilities can look up move data at runtime.
 */
UCLASS(BlueprintType)
class UCombatMoveRegistry : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Map of ability tag to move data. e.g. Ability.BasicAttack -> DA_BasicAttack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Registry")
	TMap<FGameplayTag, TObjectPtr<UCombatMoveData>> Moves;

	/** Returns the move data for the given tag, or nullptr if not found */
	UFUNCTION(BlueprintCallable, Category="Registry")
	UCombatMoveData* GetMoveData(const FGameplayTag& Tag) const;
};
