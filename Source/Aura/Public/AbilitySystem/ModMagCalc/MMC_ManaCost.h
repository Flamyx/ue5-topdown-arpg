// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ManaCost.generated.h"

/**
 * Scales a cost GE's mana modifier by the ability paying it. Set it as the modifier's Custom
 * Calculation Class, with Coefficient = the base mana cost (negative, may be curve-driven per level).
 * Result = Coefficient * UAuraGameplayAbility::GetManaCostMultiplier() of the running ability, so
 * the multiplier can follow live state such as Meteorite's charge time or Electrocute's beam count.
 */
UCLASS()
class AURA_API UMMC_ManaCost : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
