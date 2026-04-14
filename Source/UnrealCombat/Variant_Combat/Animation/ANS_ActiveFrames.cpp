#include "ANS_ActiveFrames.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"

static const FName TAG_ActiveFramesBegin("Event.ActiveFrames.Begin");
static const FName TAG_ActiveFramesEnd("Event.ActiveFrames.End");

void UANS_ActiveFrames::NotifyBegin(USkeletalMeshComponent* MeshComp,
									UAnimSequenceBase* Animation,
									float TotalDuration,
									const FAnimNotifyEventReference& EventReference)
{
	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		FGameplayEventData Payload;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner,
			FGameplayTag::RequestGameplayTag(TAG_ActiveFramesBegin),
			Payload);
	}
}

void UANS_ActiveFrames::NotifyEnd(USkeletalMeshComponent* MeshComp,
								  UAnimSequenceBase* Animation,
								  const FAnimNotifyEventReference& EventReference)
{
	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		FGameplayEventData Payload;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Owner,
			FGameplayTag::RequestGameplayTag(TAG_ActiveFramesEnd),
			Payload);
	}
}

FString UANS_ActiveFrames::GetNotifyName_Implementation() const
{
	return FString("Active Frames");
}
