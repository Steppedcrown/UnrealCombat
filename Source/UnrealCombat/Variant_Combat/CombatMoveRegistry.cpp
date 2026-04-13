// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatMoveRegistry.h"
#include "CombatMoveData.h"

UCombatMoveData* UCombatMoveRegistry::GetMoveData(const FGameplayTag& Tag) const
{
	const TObjectPtr<UCombatMoveData>* Found = Moves.Find(Tag);
	return Found ? Found->Get() : nullptr;
}
