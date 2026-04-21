// Copyright Epic Games, Inc. All Rights Reserved.

#include "LockOnComponent.h"
#include "CombatCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
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

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const FRotator DesiredRotation = UKismetMathLibrary::FindLookAtRotation(OwnerLocation, TargetLocation);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* Controller = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	if (bDebugAcquire)
	{
		UE_LOG(LogTemp, Warning, TEXT("LockOn Tick — Controller: %s | DesiredYaw: %.1f"),
			Controller ? TEXT("valid") : TEXT("NULL"),
			DesiredRotation.Yaw);
	}

	if (Controller)
	{
		const FRotator Current = Controller->GetControlRotation();
		const FRotator Smoothed = FMath::RInterpTo(Current, DesiredRotation, DeltaTime, LockOnInterpSpeed);
		Controller->SetControlRotation(Smoothed);
	}

	// keep the character yaw facing the target
	const FRotator CurrentYaw = GetOwner()->GetActorRotation();
	const FRotator TargetYaw(0.0f, DesiredRotation.Yaw, 0.0f);
	const FRotator SmoothedYaw = FMath::RInterpTo(CurrentYaw, TargetYaw, DeltaTime, LockOnInterpSpeed);
	GetOwner()->SetActorRotation(SmoothedYaw);
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

	if (bDebugAcquire)
	{
		// draw the detection sphere and forward cone direction
		DrawDebugSphere(Owner->GetWorld(), OwnerLocation, LockOnRange, 24, FColor::Yellow, false, 3.0f);
		DrawDebugDirectionalArrow(Owner->GetWorld(), OwnerLocation, OwnerLocation + ForwardVector * LockOnRange, 50.0f, FColor::Blue, false, 3.0f);
		UE_LOG(LogTemp, Warning, TEXT("LockOn: Found %d raw overlaps"), Overlaps.Num());
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACombatCharacter* Candidate = Cast<ACombatCharacter>(Overlap.GetActor());
		if (!IsValid(Candidate))
		{
			if (bDebugAcquire)
			{
				UE_LOG(LogTemp, Warning, TEXT("LockOn: Skipping overlap — not a CombatCharacter (class: %s)"),
					Overlap.GetActor() ? *Overlap.GetActor()->GetClass()->GetName() : TEXT("null"));
			}
			continue;
		}

		// filter to the forward cone
		const FVector ToCandidate = (Candidate->GetActorLocation() - OwnerLocation).GetSafeNormal();
		const float Dot = FVector::DotProduct(ForwardVector, ToCandidate);
		if (Dot < CosAngle)
		{
			if (bDebugAcquire)
			{
				DrawDebugLine(Owner->GetWorld(), OwnerLocation, Candidate->GetActorLocation(), FColor::Red, false, 3.0f);
				UE_LOG(LogTemp, Warning, TEXT("LockOn: %s rejected — outside cone (dot=%.2f, min=%.2f)"),
					*Candidate->GetName(), Dot, CosAngle);
			}
			continue;
		}

		if (bDebugAcquire)
		{
			DrawDebugLine(Owner->GetWorld(), OwnerLocation, Candidate->GetActorLocation(), FColor::Green, false, 3.0f);
			UE_LOG(LogTemp, Warning, TEXT("LockOn: %s is a valid candidate"), *Candidate->GetName());
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
		if (bDebugAcquire)
		{
			UE_LOG(LogTemp, Warning, TEXT("LockOn: No valid target found"));
		}
		return false;
	}

	if (bDebugAcquire)
	{
		DrawDebugSphere(Owner->GetWorld(), BestTarget->GetActorLocation(), 50.0f, 12, FColor::Cyan, false, 3.0f);
		UE_LOG(LogTemp, Warning, TEXT("LockOn: Locked onto %s"), *BestTarget->GetName());
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
