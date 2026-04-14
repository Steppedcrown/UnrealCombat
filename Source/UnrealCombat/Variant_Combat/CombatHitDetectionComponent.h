// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatHitDetectionComponent.generated.h"

/**
 *  Delegate broadcast once per unique hit actor during an active sweep.
 *  Phase 7 implements the full sweep logic; this declaration lets abilities bind early.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatHit, const FHitResult&);

/**
 *  Component that performs frame-by-frame weapon bone sweeps during an attack's active window.
 *
 *  Usage (by an ability):
 *    1. Bind to OnHit before calling StartTrace()
 *    2. Call StartTrace() when active frames begin (e.g. from a gameplay event)
 *    3. Call StopTrace() when active frames end
 *    4. Unbind / clear on ability end
 *
 *  Full sweep implementation lives in Phase 7 (Hit Detection & Collision).
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class UCombatHitDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCombatHitDetectionComponent();

	/**
	 *  Begin a hit-detection sweep. Clears the already-hit set so each new activation
	 *  can hit the same actors again. Called by the ability when active frames start.
	 */
	void StartTrace();

	/**
	 *  End the hit-detection sweep. Called by the ability when active frames end,
	 *  and defensively by EndAbility to ensure cleanup.
	 */
	void StopTrace();

	/** True while a sweep is in progress */
	bool IsTracing() const { return bIsTracing; }

	/** Broadcast once per unique actor hit during the active sweep */
	FOnCombatHit OnHit;

	// -----------------------------------------------------------------------
	//  Configuration — set on the component in Blueprints / Details panel
	// -----------------------------------------------------------------------

	/** Name of the weapon socket / bone from which sweeps originate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HitDetection")
	FName WeaponBoneName = NAME_None;

	/** Half-extent of the box sweep in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HitDetection", meta=(ClampMin=1))
	FVector SweepHalfExtent = FVector(20.f, 5.f, 5.f);

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

private:

	bool bIsTracing = false;

	/** Actors already struck this swing — prevents dealing damage twice to the same target */
	TArray<TObjectPtr<AActor>> HitActors;

	/** Bone world position from the previous tick, used as sweep start */
	FVector PreviousBoneLocation = FVector::ZeroVector;
};
