// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayAbility/AuraGameplayAbility.h"

#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"

FString UAuraGameplayAbility::GetDescription(int32 Level, float Damage, const FString& Title)
{
	
	float ManaCost = GetManaCost(Level);
	float Cooldown = GetCooldown(Level);
	if (Damage)
	{
		return FString::Printf(TEXT("<Title>%s</>\n<Default> Level </><Level> %d</> \n <Default>Damage: </><Damage>%.1f</> \n <Default>Cooldown:</><Cooldown>%.1f </>\n <Default>Mana Cost: </><ManaCost>%.1f</>"), *Title, Level, Damage, Cooldown, ManaCost); 
	}
	return FString::Printf(TEXT("<Title>%s</>\n<Default> Level </><Level> %d</> \n <Default>Cooldown:</><Cooldown>%.1f </>\n <Default>Mana Cost: </><ManaCost>%.1f</>"), *Title, Level, Cooldown, ManaCost); 

}

FString UAuraGameplayAbility::GetNextLevelDescription(int32 Level, float Damage)
{
	return FString::Printf(TEXT("<Default> Level </><Level>%d</>\n <Damage> Damage: %.1f</>"), Level, Damage);
}

FString UAuraGameplayAbility::GetLockedDescription(int32 Level)
{
	return FString::Printf(TEXT("<Default> Ability locked until level </> <Level>%d</>"), Level);
}

float UAuraGameplayAbility::GetManaCost(float Level) const
{
	float ManaCost = 0;
	if (auto CostGE = GetCostGameplayEffect())
	{
		for (auto Modifier: CostGE->Modifiers)
		{
			if (Modifier.Attribute == UAuraAttributeSet::GetManaAttribute())
			{
				if (Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(Level, ManaCost)) continue;

				// Scaled cost (UMMC_ManaCost): evaluate it with no ability in the context, which the
				// MMC treats as multiplier 1 - the tooltip shows the base cost, not a charge/beam state
				const FGameplayEffectContextHandle Context(UAbilitySystemGlobals::Get().AllocGameplayEffectContext());
				const FGameplayEffectSpec Spec(CostGE, Context, Level);
				Modifier.ModifierMagnitude.AttemptCalculateMagnitude(Spec, ManaCost);
			}
		}
	}
	return ManaCost;
}

float UAuraGameplayAbility::GetCooldown(float Level) const
{
	float Cooldown = 0;
	if (auto CooldownGE = GetCooldownGameplayEffect())
	{
		return CooldownGE->DurationMagnitude.GetStaticMagnitudeIfPossible(Level, Cooldown);
	}
	return Cooldown;
}
