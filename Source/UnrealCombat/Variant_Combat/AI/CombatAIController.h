// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CombatAIController.generated.h"

/**
 *  Base AI Controller for combat characters.
 *  Phase 11 adds Behavior Tree execution and Blackboard setup here.
 */
UCLASS(abstract)
class ACombatAIController : public AAIController
{
	GENERATED_BODY()

public:

	ACombatAIController();
};
