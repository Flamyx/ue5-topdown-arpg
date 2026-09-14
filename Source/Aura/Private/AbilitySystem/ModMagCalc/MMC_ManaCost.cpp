// Copyright Sparrow Inc.


#include "AbilitySystem/ModMagCalc/MMC_ManaCost.h"

#include "AbilitySystem/GameplayAbility/AuraGameplayAbility.h"

float UMMC_ManaCost::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayEffectContextHandle Context = Spec.GetContext();

	// UGameplayAbility::MakeEffectContext puts the ability in the context for CheckCost and ApplyCost.
	// Prefer the running instance, which holds the live state; the CDO is what CanActivateAbility
	// checks with before an instance exists. No ability at all (tooltips) means the base cost.
	const UAuraGameplayAbility* Ability = Cast<UAuraGameplayAbility>(Context.GetAbilityInstance_NotReplicated());
	if (Ability == nullptr)
	{
		Ability = Cast<UAuraGameplayAbility>(Context.GetAbility());
	}
	return Ability ? Ability->GetManaCostMultiplier() : 1.f;
}
