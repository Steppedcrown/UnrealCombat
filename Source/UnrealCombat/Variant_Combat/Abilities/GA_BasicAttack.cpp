// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayEffectTypes.h"
#include "CombatCharacter.h"
#include "CombatMoveData.h"
#include "CombatMoveRegistry.h"
#include "CombatHitDetectionComponent.h"

UGA_BasicAttack::UGA_BasicAttack()
{
	// Instancing policy: one instance per execution so task callbacks are safe
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Automatically apply State.Combat.Attacking while this ability is active.
	// GAS removes the tag when EndAbility fires.
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Combat.Attacking")));

	// Identity tag — required for TryActivateAbilitiesByTag to find this ability
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.BasicAttack")));
		SetAssetTags(Tags);
	}

	// Default the move lookup tag — Blueprint child can override if needed
	MoveTag = FGameplayTag::RequestGameplayTag(FName("Ability.BasicAttack"));
}

void UGA_BasicAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
										const FGameplayAbilityActorInfo* ActorInfo,
										const FGameplayAbilityActivationInfo ActivationInfo,
										const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UGA_BasicAttack: ActivateAbility called"));

	// Look up move data from the registry
	const UCombatMoveData* MoveData = GetMoveData();
	if (!MoveData)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_BasicAttack: No move data found — check DA_MoveRegistry has Ability.BasicAttack entry and BP_Player has MoveRegistry assigned"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!MoveData->AnimationMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_BasicAttack: Move data found but AnimationMontage is null — check DA_BasicAttack has a montage assigned"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("UGA_BasicAttack: Playing montage %s"), *MoveData->AnimationMontage->GetName());

	// Cache the hit detection component for use in active frame callbacks
	if (AActor* AvatarActor = ActorInfo->AvatarActor.Get())
	{
		HitDetectionComponent = AvatarActor->FindComponentByClass<UCombatHitDetectionComponent>();
		if (HitDetectionComponent)
		{
			HitDetectionComponent->OnHit.AddUObject(this, &UGA_BasicAttack::OnHitDetected);
		}
	}

	// Task 1: Play the animation montage
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MoveData->AnimationMontage);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_BasicAttack::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_BasicAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_BasicAttack::OnMontageCancelled);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_BasicAttack::OnMontageCompleted);
	MontageTask->ReadyForActivation();

	// Task 2: Listen for the active frames begin event (fired by the anim notify state)
	if (ActiveFramesBeginTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* BeginTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			ActiveFramesBeginTag);
		BeginTask->EventReceived.AddDynamic(this, &UGA_BasicAttack::OnActiveFramesBegin);
		BeginTask->ReadyForActivation();
	}

	// Task 3: Listen for the active frames end event
	if (ActiveFramesEndTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* EndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			ActiveFramesEndTag);
		EndTask->EventReceived.AddDynamic(this, &UGA_BasicAttack::OnActiveFramesEnd);
		EndTask->ReadyForActivation();
	}
}

void UGA_BasicAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
									const FGameplayAbilityActorInfo* ActorInfo,
									const FGameplayAbilityActivationInfo ActivationInfo,
									bool bReplicateEndAbility,
									bool bWasCancelled)
{
	// Make sure hit detection is stopped if the ability ends mid-swing
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

void UGA_BasicAttack::OnMontageCompleted()
{
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_BasicAttack::OnMontageCancelled()
{
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

void UGA_BasicAttack::OnMontageBlendOut(bool bInterrupted)
{
	// Handled via OnMontageCompleted / OnMontageCancelled above
}

// ---------------------------------------------------------------------------
//  Active frames callbacks
// ---------------------------------------------------------------------------

void UGA_BasicAttack::OnActiveFramesBegin(FGameplayEventData Payload)
{
	if (HitDetectionComponent)
	{
		HitDetectionComponent->StartTrace();
	}
}

void UGA_BasicAttack::OnActiveFramesEnd(FGameplayEventData Payload)
{
	if (HitDetectionComponent)
	{
		HitDetectionComponent->StopTrace();
	}
}

// ---------------------------------------------------------------------------
//  Hit detection
// ---------------------------------------------------------------------------

void UGA_BasicAttack::OnHitDetected(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return;
	}

	ApplyHitEffects(HitActor);
}

void UGA_BasicAttack::ApplyHitEffects(AActor* HitActor)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC)
	{
		return;
	}

	// Apply damage effect to target
	if (DamageEffectClass)
	{
		UAbilitySystemComponent* TargetASC = HitActor->FindComponentByClass<UAbilitySystemComponent>();
		if (TargetASC)
		{
			FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
			EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

			const FGameplayEffectSpecHandle DamageSpec =
				OwnerASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContext);

			if (DamageSpec.IsValid())
			{
				OwnerASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
			}
		}
	}

	// Apply node restore effect to self
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
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

UCombatMoveData* UGA_BasicAttack::GetMoveData() const
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
