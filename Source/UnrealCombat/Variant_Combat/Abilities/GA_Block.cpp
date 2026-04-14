// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Block.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayEffectTypes.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "CombatCharacter.h"
#include "CombatMoveData.h"
#include "CombatMoveRegistry.h"

UGA_Block::UGA_Block()
{
	// Instancing policy: one instance per actor so the timer and state are safe
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Automatically apply State.Combat.Blocking while this ability is active.
	// GAS removes the tag when EndAbility fires.
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Combat.Blocking")));

	// Default the move lookup tag — Blueprint child can override
	MoveTag = FGameplayTag::RequestGameplayTag(FName("Ability.Block"));

	// Default event tag for absorbed hits — must match what UCombatAttributeSet sends
	BlockHitEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Block.Hit"), /*bErrorIfNotFound=*/false);
}

void UGA_Block::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								const FGameplayAbilityActorInfo* ActorInfo,
								const FGameplayAbilityActivationInfo ActivationInfo,
								const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Record activation time for perfect block window comparison
	if (const UWorld* World = GetWorld())
	{
		ActivationTime = World->GetTimeSeconds();
	}

	// Optionally play the block montage from the registry
	const UCombatMoveData* MoveData = GetMoveData();
	if (MoveData && MoveData->AnimationMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			MoveData->AnimationMontage);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_Block::OnMontageCompleted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_Block::OnMontageCancelled);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_Block::OnMontageCancelled);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_Block::OnMontageCompleted);
		MontageTask->ReadyForActivation();
	}

	// Listen for the blocked-hit event fired by UCombatAttributeSet
	if (BlockHitEventTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* BlockHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			BlockHitEventTag);
		BlockHitTask->EventReceived.AddDynamic(this, &UGA_Block::OnDamageBlocked);
		BlockHitTask->ReadyForActivation();
	}

	// Start the block window timer — ability ends when this fires
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BlockWindowTimerHandle,
			this,
			&UGA_Block::OnBlockWindowExpired,
			BlockWindowDuration,
			/*bLoop=*/false);
	}
}

void UGA_Block::EndAbility(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo,
							bool bReplicateEndAbility,
							bool bWasCancelled)
{
	// Clear the block window timer if ability is ended early (e.g. interrupted)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlockWindowTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ---------------------------------------------------------------------------
//  Timer callback
// ---------------------------------------------------------------------------

void UGA_Block::OnBlockWindowExpired()
{
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// ---------------------------------------------------------------------------
//  Blocked hit callback
// ---------------------------------------------------------------------------

void UGA_Block::OnDamageBlocked(FGameplayEventData Payload)
{
	// Check if this hit arrived within the perfect block window
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float TimeSinceActivation = World->GetTimeSeconds() - ActivationTime;
	if (TimeSinceActivation <= PerfectBlockWindowDuration)
	{
		AActor* Attacker = const_cast<AActor*>(Payload.Instigator.Get());
		TriggerPerfectBlock(Attacker);
	}
}

// ---------------------------------------------------------------------------
//  Perfect block
// ---------------------------------------------------------------------------

void UGA_Block::TriggerPerfectBlock(AActor* Attacker)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC)
	{
		return;
	}

	// Restore a Node to self
	if (NodeRestoreEffectClass)
	{
		FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
		EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

		const FGameplayEffectSpecHandle RestoreSpec =
			OwnerASC->MakeOutgoingSpec(NodeRestoreEffectClass, GetAbilityLevel(), EffectContext);

		if (RestoreSpec.IsValid())
		{
			OwnerASC->ApplyGameplayEffectSpecToSelf(*RestoreSpec.Data.Get());
		}
	}

	// Apply knockback to all enemies within PerfectBlockRadius
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || !KnockbackEffectClass)
	{
		return;
	}

	const FVector Origin = AvatarActor->GetActorLocation();
	const UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		return;
	}

	// Sphere overlap to find nearby actors
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);

	World->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(PerfectBlockRadius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (OverlapActor && OverlapActor->FindComponentByClass<UAbilitySystemComponent>())
		{
			ApplyKnockbackToTarget(OverlapActor);
		}
	}
}

void UGA_Block::ApplyKnockbackToTarget(AActor* Target)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = Target ? Target->FindComponentByClass<UAbilitySystemComponent>() : nullptr;

	if (!OwnerASC || !TargetASC || !KnockbackEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

	const FGameplayEffectSpecHandle KnockbackSpec =
		OwnerASC->MakeOutgoingSpec(KnockbackEffectClass, GetAbilityLevel(), EffectContext);

	if (KnockbackSpec.IsValid())
	{
		OwnerASC->ApplyGameplayEffectSpecToTarget(*KnockbackSpec.Data.Get(), TargetASC);
	}
}

// ---------------------------------------------------------------------------
//  Montage callbacks
// ---------------------------------------------------------------------------

void UGA_Block::OnMontageCompleted()
{
	// Montage finishing on its own doesn't end the ability — the block window timer does.
	// No action needed here.
}

void UGA_Block::OnMontageCancelled()
{
	// If the montage is forcibly cancelled (e.g. by another ability), end the block ability too.
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

UCombatMoveData* UGA_Block::GetMoveData() const
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
