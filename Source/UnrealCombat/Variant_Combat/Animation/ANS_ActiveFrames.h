#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "ANS_ActiveFrames.generated.h"

/**
 * Anim Notify State that fires GAS gameplay events at the start and end of
 * a montage's active hit window. Abilities listen for these events via
 * UAbilityTask_WaitGameplayEvent to gate hit detection.
 */
UCLASS()
class UNREALCOMBAT_API UANS_ActiveFrames : public UAnimNotifyState
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
