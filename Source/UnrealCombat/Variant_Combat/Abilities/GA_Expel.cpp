// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_Expel.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameplayEffectTypes.h"
#include "CombatCharacter.h"
#include "CombatMoveData.h"
#include "CombatMoveRegistry.h"
#include "CombatHitDetectionComponent.h"
#include "CombatAttributeSet.h"

UGA_Expel::UGA_Expel()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Combat.Attacking")));

	// Identity tag — required for TryActivateAbilitiesByTag to find this ability
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Expel")));
		SetAssetTags(Tags);
	}

	MoveTag = FGameplayTag::RequestGameplayTag(FName("Ability.Expel"));

	ActiveFramesBeginTag = FGameplayTag::RequestGameplayTag(FName("Event.ActiveFrames.Begin"), /*bErrorIfNotFound=*/false);
	ActiveFramesEndTag   = FGameplayTag::RequestGameplayTag(FName("Event.ActiveFrames.End"),   /*bErrorIfNotFound=*/false);
}

// ---------------------------------------------------------------------------
//  CanActivateAbility
// ---------------------------------------------------------------------------

bool UGA_Expel::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
		// No cost defined — allow activation
		return true;
	}

	// Check that the caster has at least NodeCost Nodes available
	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return false;
	}

	const UCombatAttributeSet* AttrSet = ASC->GetSet<UCombatAttributeSet>();
	if (!AttrSet)
	{
		return false;
	}

	return (AttrSet->GetNodes() + AttrSet->GetTempNodes()) >= static_cast<float>(MoveData->NodeCost);
}

// ---------------------------------------------------------------------------
//  ActivateAbility
// ---------------------------------------------------------------------------

void UGA_Expel::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								const FGameplayAbilityActorInfo* ActorInfo,
								const FGameplayAbilityActivationInfo ActivationInfo,
								const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Consume Nodes immediately on activation
	ConsumeNodes();

	// Look up move data
	const UCombatMoveData* MoveData = GetMoveData();
	if (!MoveData || !MoveData->AnimationMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_Expel: No move data or animation montage for tag %s"),
			*MoveTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Cache hit detection component and bind
	if (AActor* AvatarActor = ActorInfo->AvatarActor.Get())
	{
		HitDetectionComponent = AvatarActor->FindComponentByClass<UCombatHitDetectionComponent>();
		if (HitDetectionComponent)
		{
			HitDetectionComponent->OnHit.AddUObject(this, &UGA_Expel::OnHitDetected);
		}
	}

	// Task 1: Play montage
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MoveData->AnimationMontage);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Expel::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Expel::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Expel::OnMontageCancelled);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Expel::OnMontageCompleted);
	MontageTask->ReadyForActivation();

	// Task 2: Listen for active frames begin
	if (ActiveFramesBeginTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* BeginTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			ActiveFramesBeginTag);
		BeginTask->EventReceived.AddDynamic(this, &UGA_Expel::OnActiveFramesBegin);
		BeginTask->ReadyForActivation();
	}

	// Task 3: Listen for active frames end
	if (ActiveFramesEndTag.IsValid())
	{
		UAbilityTask_WaitGameplayEvent* EndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			ActiveFramesEndTag);
		EndTask->EventReceived.AddDynamic(this, &UGA_Expel::OnActiveFramesEnd);
		EndTask->ReadyForActivation();
	}
}

void UGA_Expel::EndAbility(const FGameplayAbilitySpecHandle Handle,
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

void UGA_Expel::OnMontageCompleted()
{
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Expel::OnMontageCancelled()
{
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

// ---------------------------------------------------------------------------
//  Active frames callbacks
// ---------------------------------------------------------------------------

void UGA_Expel::OnActiveFramesBegin(FGameplayEventData Payload)
{
	if (HitDetectionComponent)
	{
		if (const UCombatMoveData* MoveData = GetMoveData())
		{
			HitDetectionComponent->WeaponBoneName  = MoveData->WeaponBoneName;
			HitDetectionComponent->SweepHalfExtent = MoveData->SweepHalfExtent;
		}
		HitDetectionComponent->StartTrace();
	}
}

void UGA_Expel::OnActiveFramesEnd(FGameplayEventData Payload)
{
	if (HitDetectionComponent)
	{
		HitDetectionComponent->StopTrace();
	}
}

// ---------------------------------------------------------------------------
//  Hit detection
// ---------------------------------------------------------------------------

void UGA_Expel::OnHitDetected(const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (!HitActor)
	{
		return;
	}

	ApplyHitEffects(HitActor);
}

void UGA_Expel::ApplyHitEffects(AActor* HitActor)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = HitActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = OwnerASC->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());

	// Apply damage to target
	if (DamageEffectClass)
	{
		const FGameplayEffectSpecHandle DamageSpec =
			OwnerASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContext);

		if (DamageSpec.IsValid())
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		}
	}

	// Apply temp node effect to target
	if (TempNodeEffectClass)
	{
		const FGameplayEffectSpecHandle TempNodeSpec =
			OwnerASC->MakeOutgoingSpec(TempNodeEffectClass, GetAbilityLevel(), EffectContext);

		if (TempNodeSpec.IsValid())
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*TempNodeSpec.Data.Get(), TargetASC);
		}
	}
}

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

void UGA_Expel::ConsumeNodes()
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

	// Consume TempNodes first by removing active Effect.TempNode duration GEs,
	// then fall through to regular Nodes for the remainder
	int32 Remaining = Cost;
	{
		const UCombatAttributeSet* AttrSet = OwnerASC->GetSet<UCombatAttributeSet>();
		const int32 TempConsumed = AttrSet ? FMath::Min(Remaining, FMath::FloorToInt(AttrSet->GetTempNodes())) : 0;
		if (TempConsumed > 0 && TempNodeEffectClassToRemove)
		{
			FGameplayEffectQuery Query;
			const TSubclassOf<UGameplayEffect> ClassToMatch = TempNodeEffectClassToRemove;
			Query.CustomMatchDelegate.BindLambda([ClassToMatch](const FActiveGameplayEffect& Effect)
			{
				return Effect.Spec.Def && Effect.Spec.Def->GetClass() == ClassToMatch;
			});
			const int32 ActuallyRemoved = OwnerASC->RemoveActiveEffects(Query, TempConsumed);
			Remaining -= ActuallyRemoved;
		}
	}

	for (int32 i = 0; i < Remaining; ++i)
	{
		const FGameplayEffectSpecHandle ConsumeSpec =
			OwnerASC->MakeOutgoingSpec(ConsumeNodeEffectClass, GetAbilityLevel(), EffectContext);
		if (ConsumeSpec.IsValid())
		{
			OwnerASC->ApplyGameplayEffectSpecToSelf(*ConsumeSpec.Data.Get());
		}
	}
}

UCombatMoveData* UGA_Expel::GetMoveData() const
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
