// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CombatMoveData.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundBase;

/**
 *  Data asset describing a single combat move.
 *  Referenced by UCombatMoveRegistry and looked up at runtime by abilities via gameplay tag.
 */
UCLASS(BlueprintType)
class UCombatMoveData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Damage dealt to the target on a successful hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Damage")
	float Damage = 1.0f;

	/** Number of Nodes consumed from the caster on activation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Nodes")
	int32 NodeCost = 0;

	/** Number of Nodes restored to the caster on a successful hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Nodes")
	int32 NodeGain = 0;

	/** Frames before the move becomes active (input to first active frame) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Frames", meta=(ClampMin=0))
	int32 StartupFrames = 0;

	/** Frames the hitbox is active */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Frames", meta=(ClampMin=0))
	int32 ActiveFrames = 0;

	/** Frames of recovery after the active window before the character is actionable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Frames", meta=(ClampMin=0))
	int32 RecoveryFrames = 0;

	/**
	 *  Skeleton bone or socket the hit detection sweep originates from.
	 *  e.g. "hand_r" for a punch, "foot_r" for a kick.
	 *  Leave as None to fall back to the mesh root (not recommended).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|HitDetection")
	FName WeaponBoneName = NAME_None;

	/** Half-extent of the sweep box in cm. Overrides the component default for this move. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|HitDetection", meta=(ClampMin=1))
	FVector SweepHalfExtent = FVector(15.f, 10.f, 10.f);

	/** Montage to play when this move is activated */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Assets")
	TObjectPtr<UAnimMontage> AnimationMontage;

	/** Niagara system to spawn at the hit point */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Assets")
	TObjectPtr<UNiagaraSystem> HitEffect;

	/** Sound to play on a successful hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Assets")
	TObjectPtr<USoundBase> HitSound;
};
