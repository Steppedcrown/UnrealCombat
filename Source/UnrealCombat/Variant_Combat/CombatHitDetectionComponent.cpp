// Copyright Epic Games, Inc. All Rights Reserved.

#include "CombatHitDetectionComponent.h"

UCombatHitDetectionComponent::UCombatHitDetectionComponent()
{
	// Tick is required for the frame-by-frame sweep, but disabled until tracing starts
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCombatHitDetectionComponent::StartTrace()
{
	// Clear previously-hit actors so each new activation starts fresh
	HitActors.Reset();

	// Seed the previous bone location to avoid a phantom sweep on the first tick
	if (AActor* Owner = GetOwner())
	{
		if (USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>())
		{
			PreviousBoneLocation = WeaponBoneName.IsNone()
				? Mesh->GetComponentLocation()
				: Mesh->GetSocketLocation(WeaponBoneName);
		}
	}

	bIsTracing = true;
	SetComponentTickEnabled(true);
}

void UCombatHitDetectionComponent::StopTrace()
{
	bIsTracing = false;
	SetComponentTickEnabled(false);
}

void UCombatHitDetectionComponent::TickComponent(float DeltaTime,
												  ELevelTick TickType,
												  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsTracing)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh)
	{
		return;
	}

	// Current weapon bone world position
	const FVector CurrentBoneLocation = WeaponBoneName.IsNone()
		? Mesh->GetComponentLocation()
		: Mesh->GetSocketLocation(WeaponBoneName);

	// -----------------------------------------------------------------------
	//  Phase 7 — full box sweep implementation goes here.
	//  For now the sweep is a no-op stub so GA_BasicAttack can compile and
	//  bind to OnHit without crashes. Replace with the real sweep in Phase 7:
	//
	//    FCollisionShape Box;
	//    Box.SetBox(SweepHalfExtent);
	//    TArray<FHitResult> Hits;
	//    GetWorld()->SweepMultiByObjectType(Hits, PreviousBoneLocation,
	//        CurrentBoneLocation, FQuat::Identity, ObjectParams, Box, QueryParams);
	//    for (auto& Hit : Hits) { ... OnHit.Broadcast(Hit); }
	// -----------------------------------------------------------------------

	PreviousBoneLocation = CurrentBoneLocation;
}
