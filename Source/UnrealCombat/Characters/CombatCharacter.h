#pragma once

#include "CoreMinimal.h"
#include "UnrealCombatCharacter.h"
#include "AbilitySystemInterface.h"
#include "CombatCharacter.generated.h"

class UAbilitySystemComponent;
class UMotionWarpingComponent;

/**
 * Base class for all combat characters (player and enemy).
 * Extends AUnrealCombatCharacter with the Gameplay Ability System and Motion Warping.
 */
UCLASS()
class UNREALCOMBAT_API ACombatCharacter : public AUnrealCombatCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	ACombatCharacter();

	/** IAbilitySystemInterface */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	FORCEINLINE UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

protected:

	/** Called on the server when a controller possesses this character. Initialises ASC actor info. */
	virtual void PossessedBy(AController* NewController) override;

	/** Called on clients when PlayerState replicates. Initialises ASC actor info for the owning client. */
	virtual void OnRep_PlayerState() override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UMotionWarpingComponent* MotionWarpingComponent;
};
