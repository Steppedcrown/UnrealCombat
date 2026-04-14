// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotifyState_ComboWindow.h"
#include "CombatAttacker.h"
#include "Components/SkeletalMeshComponent.h"

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp,
												UAnimSequenceBase* Animation,
												float TotalDuration,
												const FAnimNotifyEventReference& EventReference)
{
	if (ICombatAttacker* Attacker = Cast<ICombatAttacker>(MeshComp->GetOwner()))
	{
		Attacker->OpenComboWindow();
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp,
											  UAnimSequenceBase* Animation,
											  const FAnimNotifyEventReference& EventReference)
{
	if (ICombatAttacker* Attacker = Cast<ICombatAttacker>(MeshComp->GetOwner()))
	{
		Attacker->CloseComboWindow();
	}
}

FString UAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return FString("Combo Window");
}
