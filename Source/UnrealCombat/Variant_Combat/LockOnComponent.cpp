// Copyright Epic Games, Inc. All Rights Reserved.

#include "LockOnComponent.h"
#include "CombatCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	// cache the owner's spring arm
	if (ACombatCharacter* Owner = Cast<ACombatCharacter>(GetOwner()))
	{
		CameraBoom = Owner->GetCameraBoom();
	}
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// if the target has somehow become invalid, clear and stop
	if (!IsValid(TargetActor))
	{
		ClearTarget();
		return;
	}

	if (!CameraBoom)
	{
		return;
	}

	// rotate the spring arm smoothly toward the target
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FRotator DesiredRotation = UKismetMathLibrary::FindLookAtRotation(OwnerLocation, TargetLocation);
	const FRotator SmoothedRotation = FMath::RInterpTo(CameraBoom->GetComponentRotation(), DesiredRotation, DeltaTime, LockOnInterpSpeed);

	GetOwner()->GetInstigatorController()->SetControlRotation(SmoothedRotation);
}

void ULockOnComponent::ToggleLockOn()
{
	if (bIsLocked)
	{
		ClearTarget();
	}
	else
	{
		AcquireTarget();
	}
}

bool ULockOnComponent::AcquireTarget()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector ForwardVector = Owner->GetActorForwardVector();

	// sphere overlap for all pawns within LockOnRange
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(LockOnRange);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	Owner->GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		OwnerLocation,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Sphere,
		QueryParams
	);

	ACombatCharacter* BestTarget = nullptr;
	float BestDistanceSq = FLT_MAX;
	const float CosAngle = FMath::Cos(FMath::DegreesToRadians(LockOnAngle));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACombatCharacter* Candidate = Cast<ACombatCharacter>(Overlap.GetActor());
		if (!IsValid(Candidate))
		{
			continue;
		}

		// filter to the forward cone
		const FVector ToCandidate = (Candidate->GetActorLocation() - OwnerLocation).GetSafeNormal();
		if (FVector::DotProduct(ForwardVector, ToCandidate) < CosAngle)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(OwnerLocation, Candidate->GetActorLocation());
		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestTarget = Candidate;
		}
	}

	if (!BestTarget)
	{
		return false;
	}

	TargetActor = BestTarget;
	bIsLocked = true;

	// listen for target death so we can auto-clear
	TargetActor->OnDestroyed.AddDynamic(this, &ULockOnComponent::OnTargetDestroyed);

	// enable tick now that we have a target
	SetComponentTickEnabled(true);

	return true;
}

void ULockOnComponent::ClearTarget()
{
	if (IsValid(TargetActor))
	{
		TargetActor->OnDestroyed.RemoveDynamic(this, &ULockOnComponent::OnTargetDestroyed);
	}

	TargetActor = nullptr;
	bIsLocked = false;

	SetComponentTickEnabled(false);
}

void ULockOnComponent::OnTargetDestroyed(AActor* DestroyedActor)
{
	ClearTarget();
}
