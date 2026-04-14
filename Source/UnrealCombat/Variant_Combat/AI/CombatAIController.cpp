// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatAIController.h"

ACombatAIController::ACombatAIController()
{
	// Start AI logic (Behavior Tree) automatically on possess
	bStartAILogicOnPossess = true;

	// Attach to the possessed pawn so EQS queries work correctly
	bAttachToPawn = true;
}
