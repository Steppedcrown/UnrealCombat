// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ComboWindow.generated.h"

/**
 *  Defines the window in which a combo input is accepted.
 *
 *  Place this notify state over the recovery/transition frames of each attack section
 *  in the combo montage (typically after the active hit frames, before the animation
 *  would naturally end).
 *
 *  - NotifyBegin: opens the window on the owning character. Any input buffered before
 *                 this point is applied immediately; new input during the window chains
 *                 the combo on the next available frame.
 *  - NotifyEnd:   closes the window. Input arriving after this point is ignored until
 *                 the next attack or the AttackMontageEnded fallback fires.
 */
UCLASS()
class UAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
							  UAnimSequenceBase* Animation,
							  float TotalDuration,
							  const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
						   UAnimSequenceBase* Animation,
						   const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
