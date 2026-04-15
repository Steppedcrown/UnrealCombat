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

	// Orient the box along the sweep direction so it tracks the weapon arc naturally
	const FVector SweepDelta = CurrentBoneLocation - PreviousBoneLocation;
	const FQuat SweepRotation = SweepDelta.IsNearlyZero()
		? FQuat::Identity
		: SweepDelta.ToOrientationQuat();

	FCollisionShape Box;
	Box.SetBox(FVector3f(SweepHalfExtent));

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bTraceComplex = false;

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByProfile(
		Hits,
		PreviousBoneLocation,
		CurrentBoneLocation,
		SweepRotation,
		FName("Profile_Hitbox"),
		Box,
		QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActors.Contains(HitActor))
		{
			continue;
		}

		HitActors.Add(HitActor);
		OnHit.Broadcast(Hit);
	}

	PreviousBoneLocation = CurrentBoneLocation;
}
