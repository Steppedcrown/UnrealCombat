// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Rip.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayEffectTypes.h"
#include "CombatCharacter.h"
#include "CombatMoveData.h"
#include "CombatMoveRegistry.h"
#include "CombatHitDetectionComponent.h"
#include "CombatAttributeSet.h"

UGA_Rip::UGA_Rip()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Combat.Attacking")));

	MoveTag = FGameplayTag::RequestGameplayTag(FName("Ability.Rip"));

	ActiveFramesBeginTag = FGameplayTag::RequestGameplayTag(FName("Event.ActiveFrames.Begin"), /*bErrorIfNotFound=*/false);
	ActiveFramesEndTag   = FGameplayTag::RequestGameplayTag(FName("Event.ActiveFrames.End"),   /*bErrorIfNotFound=*/false);
}

// ---------------------------------------------------------------------------
//  CanActivateAbility
// ---------------------------------------------------------------------------

bool UGA_Rip::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
								  const FGameplayAbilityActorInfo* ActorInfo,
								  const FGameplayTagContainer* SourceTags,
								  const FGameplayTagContainer* TargetTags,
								  FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UCombatMoveData* MoveData = GetMoveData();
	if (!MoveData || MoveData->NodeCost <= 0)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const UCombatAttributeSet* AttrSet = ASC ? ASC->GetSet<UCombatAttributeSet>() : nullptr;
	if (!AttrSet)
	{
		return false;
	}

	return AttrSet->GetNodes() >= static_cast<float>(MoveData->NodeCost);
}

// ---------------------------------------------------------------------------
//  ActivateAbility
// ---------------------------------------------------------------------------

void UGA_Rip::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
							   const FGameplayAbilityActorInfo* ActorInfo,
							   const FGameplayAbilityActivationInfo ActivationInfo,
							   const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ConsumeNodes();

	const UCombatMoveData* MoveData = GetMoveData();
	if (!MoveData || !MoveData->AnimationMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_Rip: No move data or animation montage for tag %s"), *MoveTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AActor* AvatarActor = ActorInfo->AvatarActor.Get())
	{
		HitDetectionComponent = AvatarActor->FindComponentByClass<UCombatHitDetectionComponent>();
		if (HitDetectionComponent)
		{
			HitDetectionComponent->OnHit.AddUObject(this, &UGA_Rip::OnHitDetected);
		}
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MoveData->AnimationMontage);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Rip::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Rip::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Rip::OnMontageCancelled);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Rip::OnMontageCompleted);
	MontageTask->ReadyForActivation();

	if (ActiveFramesBeginTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* BeginTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ActiveFramesBeginTag);
		BeginTask->EventReceived.AddDynamic(this, &UGA_Rip::OnActiveFramesBegin);
		BeginTask->ReadyForActivation();
	}

	if (ActiveFramesEndTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* EndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ActiveFramesEndTag);
		EndTask->EventReceived.AddDynamic(this, &UGA_Rip::OnActiveFramesEnd);
		EndTask->ReadyForActivation();
	}
}

void UGA_Rip::EndAbility(const FGameplayAbilitySpecHandle Handle,
						  const FGameplayAbilityActorInfo* ActorInfo,
						  const FGameplayAbilityActivationInfo ActivationInfo,
						  bool bReplicateEndAbility,
						  bool bWasCancelled)
{
	if (HitDetectionComponent)
	{
		HitDetectionComponent->StopTrace();
		HitDetectionComponent->OnHit.RemoveAll(this);
		HitDetectionComponent = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
//  Montage callbacks
// ---------------------------------------------------------------------------

void UGA_Rip::OnMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_Rip::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

// ---------------------------------------------------------------------------
//  Active frames callbacks
// ---------------------------------------------------------------------------

void UGA_Rip::OnActiveFramesBegin(FGameplayEventData Payload)
{
	if (HitDetectionComponent)
	{
		HitDetectionComponent->StartTrace();
	}
}

void UGA_Rip::OnActiveFramesEnd(FGameplayEventData Payload)
{
	if (HitDetectionComponent)
	{
		HitDetectionComponent->StopTrace();
	}
}

// ---------------------------------------------------------------------------
//  Hit detection
// ---------------------------------------------------------------------------

void UGA_Rip::OnHitDetected(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (HitActor)
	{
		ApplyHitEffects(HitActor);
	}
}

void UGA_Rip::ApplyHitEffects(AActor* HitActor)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = HitActor->FindComponentByClass<UAbilitySystemComponent>();

	if (!OwnerASC || !TargetASC)
	{
		return;
	}

	const FGameplayTag VulnerableTag = FGameplayTag::RequestGameplayTag(FName("State.Status.Vulnerable"));

	if (TargetASC->HasMatchingGameplayTag(VulnerableTag))
	{
		// Target is vulnerable — kill them
		if (KillEffectClass)
		{
			FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
			EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

			const FGameplayEffectSpecHandle KillSpec =
				OwnerASC->MakeOutgoingSpec(KillEffectClass, GetAbilityLevel(), EffectContext);
			if (KillSpec.IsValid())
			{
				OwnerASC->ApplyGameplayEffectSpecToTarget(*KillSpec.Data.Get(), TargetASC);
			}
		}
	}
	else
	{
		// Target is not vulnerable — deal minor damage and steal nodes
		FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
		EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

		if (DamageEffectClass)
		{
			const FGameplayEffectSpecHandle DamageSpec =
				OwnerASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContext);
			if (DamageSpec.IsValid())
			{
				OwnerASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
			}
		}

		StealNodes(HitActor, TargetASC);
	}
}

void UGA_Rip::StealNodes(AActor* TargetActor, UAbilitySystemComponent* TargetASC)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC || !TargetASC)
	{
		return;
	}

	const UCombatAttributeSet* TargetAttrSet = TargetASC->GetSet<UCombatAttributeSet>();
	if (!TargetAttrSet)
	{
		return;
	}

	const int32 NodesToSteal = FMath::FloorToInt(TargetAttrSet->GetNodes());
	if (NodesToSteal <= 0)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

	for (int32 i = 0; i < NodesToSteal; ++i)
	{
		if (StealNodeFromTargetEffectClass)
		{
			const FGameplayEffectSpecHandle StealSpec =
				OwnerASC->MakeOutgoingSpec(StealNodeFromTargetEffectClass, GetAbilityLevel(), EffectContext);
			if (StealSpec.IsValid())
			{
				OwnerASC->ApplyGameplayEffectSpecToTarget(*StealSpec.Data.Get(), TargetASC);
			}
		}

		if (RestoreNodeEffectClass)
		{
			const FGameplayEffectSpecHandle RestoreSpec =
				OwnerASC->MakeOutgoingSpec(RestoreNodeEffectClass, GetAbilityLevel(), EffectContext);
			if (RestoreSpec.IsValid())
			{
				OwnerASC->ApplyGameplayEffectSpecToSelf(*RestoreSpec.Data.Get());
			}
		}
	}
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

void UGA_Rip::ConsumeNodes()
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC || !ConsumeNodeEffectClass)
	{
		return;
	}

	const UCombatMoveData* MoveData = GetMoveData();
	const int32 Cost = MoveData ? MoveData->NodeCost : 1;

	FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

	for (int32 i = 0; i < Cost; ++i)
	{
		const FGameplayEffectSpecHandle ConsumeSpec =
			OwnerASC->MakeOutgoingSpec(ConsumeNodeEffectClass, GetAbilityLevel(), EffectContext);
		if (ConsumeSpec.IsValid())
		{
			OwnerASC->ApplyGameplayEffectSpecToSelf(*ConsumeSpec.Data.Get());
		}
	}
}

UCombatMoveData* UGA_Rip::GetMoveData() const
{
	const ACombatCharacter* CombatChar = Cast<ACombatCharacter>(GetAvatarActorFromActorInfo());
	if (!CombatChar)
	{
		return nullptr;
	}

	const UCombatMoveRegistry* Registry = CombatChar->GetMoveRegistry();
	if (!Registry)
	{
		return nullptr;
	}

	return Registry->GetMoveData(MoveTag);
}
