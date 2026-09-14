// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * AuraGameplayTags; Singleton containing native (created in c++) Gameplay Tags
 */

struct FAuraGameplayTags
{
public:
	static const FAuraGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();
	
	FGameplayTag Player_Block_CursorTrace;
	FGameplayTag Player_Block_InputPressed;
	FGameplayTag Player_Block_InputHeld;
	FGameplayTag Player_Block_InputReleased;

	FGameplayTag Attributes_Primary_Strength;
	FGameplayTag Attributes_Primary_Intelligence;
	FGameplayTag Attributes_Primary_Resilience;
	FGameplayTag Attributes_Primary_Vigor;

	FGameplayTag Attributes_Secondary_MaxHealth;
	FGameplayTag Attributes_Secondary_MaxMana;
	
	FGameplayTag Attributes_Secondary_Armor;
	FGameplayTag Attributes_Secondary_ArmorPenetration;
	FGameplayTag Attributes_Secondary_BlockChance;
	FGameplayTag Attributes_Secondary_CriticalHitChance;
	FGameplayTag Attributes_Secondary_CriticalHitDamage;
	FGameplayTag Attributes_Secondary_CriticalHitResistance;
	FGameplayTag Attributes_Secondary_HealthRegeneration;
	FGameplayTag Attributes_Secondary_ManaRegeneration;

	FGameplayTag Attributes_Resistance_Fire;
	FGameplayTag Attributes_Resistance_Lightning;
	FGameplayTag Attributes_Resistance_Arcane;
	FGameplayTag Attributes_Resistance_Physical;
	
	FGameplayTag Debuff_Burn;
	FGameplayTag Debuff_Stun;
	
	FGameplayTag Debuff_Chance;
	FGameplayTag Debuff_Damage;
	FGameplayTag Debuff_Duration;
	FGameplayTag Debuff_Frequency;
	
	FGameplayTag Attributes_Meta_IncomingXP;

	FGameplayTag InputTag_RMB;
	FGameplayTag InputTag_LMB;
	FGameplayTag InputTag_1;
	FGameplayTag InputTag_2;
	FGameplayTag InputTag_3;
	FGameplayTag InputTag_4;

	FGameplayTag Damage;
	FGameplayTag Damage_Fire;
	FGameplayTag Damage_Lightning;
	FGameplayTag Damage_Arcane;
	FGameplayTag Damage_Physical;
	FGameplayTag Damage_Residual_Fire;

	FGameplayTag Abilities_Attack;
	FGameplayTag Abilities_Summon;
	FGameplayTag Abilities_Fire_FireBolt;
	FGameplayTag Abilities_Fire_Meteorite;
	FGameplayTag Abilities_Lightning_Electrocute;
	FGameplayTag Abilities_Passive_AuraBuff;
	FGameplayTag Abilities_None;
	
	FGameplayTag AbilityType_Passive;
	FGameplayTag AbilityType_Offensive;

	FGameplayTag Cooldown_Fire_FireBolt;
	FGameplayTag Cooldown_Fire_Meteorite;
	FGameplayTag Cooldown_Lightning_Electrocute;

	FGameplayTag HitReact;
	FGameplayTag Cooldown;
	FGameplayTag Knockback;
	FGameplayTag DeathImpulse;
	
	FGameplayTag Status_Equipped;
	FGameplayTag Status_Unlocked;
	FGameplayTag Status_Eligible;
	FGameplayTag Status_Locked;

	FGameplayTag CombatSocket_Weapon;
	FGameplayTag CombatSocket_RightHand;
	FGameplayTag CombatSocket_LeftHand;

	FGameplayTag Montage_Attack_1;
	FGameplayTag Montage_Attack_2;
	FGameplayTag Montage_Attack_3;
	FGameplayTag Montage_Attack_4;

	FGameplayTag Enemy;
	FGameplayTag Aura;
	
	FGameplayTag Effect_Status_Applied;

	FGameplayTag GameplayCue_MeleeImpact;
	FGameplayTag GameplayCue_ShockBurst;
	FGameplayTag GameplayCue_ShockLoop;
	FGameplayTag GameplayCue_ShockLoop_Fork;

	TMap<FGameplayTag, FGameplayTag> DamageTypesToResistances;

private:
	static FAuraGameplayTags GameplayTags;
};