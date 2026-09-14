// Fill out your copyright notice in the Description page of Project Settings.


#include "AuraGameplayTags.h"
#include "GameplayTagsManager.h"

FAuraGameplayTags FAuraGameplayTags::GameplayTags;

void FAuraGameplayTags::InitializeNativeGameplayTags()
{
	/*
	Player tags
	*/
	GameplayTags.Player_Block_CursorTrace= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.CursorTrace"),
		FString("Block tracing under the cursor"));
	GameplayTags.Player_Block_InputHeld= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputHeld"),
		FString("Block tracing under the cursor"));
	GameplayTags.Player_Block_InputReleased= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputReleased"),
		FString("Block tracing under the cursor"));
	GameplayTags.Player_Block_InputPressed= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Player.Block.InputPressed"),
		FString("Block tracing under the cursor"));
	
	/*
	Primary attributes
	*/
	GameplayTags.Attributes_Primary_Strength= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Strength"),
		FString("Increases physical damage"));

	GameplayTags.Attributes_Primary_Intelligence = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Intelligence"),
		FString("Increases magical damage"));

	GameplayTags.Attributes_Primary_Resilience = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Resilience"),
		FString("Increases armor and armor penetration"));

	GameplayTags.Attributes_Primary_Vigor = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Primary.Vigor"),
		FString("Increases health"));

	/*
	Secondary attributes
	*/
	GameplayTags.Attributes_Secondary_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.MaxHealth"),
		FString("Maximum amount of Health obtainable"));

	GameplayTags.Attributes_Secondary_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.MaxMana"),
		FString("Maximum amount of Mana obtainable"));

	GameplayTags.Attributes_Secondary_Armor = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.Armor"),
		FString("Reduces damage taken; improves Block Chance"));

	GameplayTags.Attributes_Secondary_ArmorPenetration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.ArmorPenetration"),
		FString("Ignores Percentage of enemy's armor, increases critical hit chance"));

	GameplayTags.Attributes_Secondary_BlockChance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.BlockChance"),
		FString("Chance to cut incoming damage in half"));

	GameplayTags.Attributes_Secondary_CriticalHitChance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitChance"),
		FString("Chance to double damage plus critical hit bonus"));

	GameplayTags.Attributes_Secondary_CriticalHitDamage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitDamage"),
		FString("Bonus damage when critical hit is applied"));

	GameplayTags.Attributes_Secondary_CriticalHitResistance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.CriticalHitResistance"),
		FString("Reduces enemy's critical hit chance"));

	GameplayTags.Attributes_Secondary_HealthRegeneration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.HealthRegeneration"),
		FString("Amount of Health regenerated per time unit"));

	GameplayTags.Attributes_Secondary_ManaRegeneration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Secondary.ManaRegeneration"),
		FString("Amount of Mana regenerated per time unit"));

	/*
	Meta Attributes
	*/
	GameplayTags.Attributes_Meta_IncomingXP = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Meta.IncomingXP"),
		FString("Incoming XP Meta Attribute"));

	/*
	Input action tags
	*/
	GameplayTags.InputTag_LMB = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.LMB"),
		FString("Input Tag for Left Mouse Burron"));
	GameplayTags.InputTag_RMB = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.RMB"),
		FString("Input Tag for Right Mouse Burron"));
	GameplayTags.InputTag_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.1"),
		FString("Input Tag for 1"));
	GameplayTags.InputTag_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.2"),
		FString("Input Tag for 2"));
	GameplayTags.InputTag_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.3"),
		FString("Input Tag for 3"));
	GameplayTags.InputTag_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("InputTag.4"),
		FString("Input Tag for 4"));

	/* 
	Damage types
	*/

	GameplayTags.Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage"),
		FString("Damage"));

	GameplayTags.Damage_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Fire"),
		FString("Fire damage type"));

	GameplayTags.Damage_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Lightning"),
		FString("Lightning damage type"));

	GameplayTags.Damage_Arcane= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Arcane"),
		FString("Arcane damage type"));

	GameplayTags.Damage_Physical= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Physical"),
		FString("Physical damage type"));

	GameplayTags.Damage_Residual_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Residual.Fire"),
		FString("Residual damage fire"));

	/*
	Resistances
	*/

	GameplayTags.Attributes_Resistance_Fire = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Fire"),
		FString("Fire damage resistance")
	);

	GameplayTags.Attributes_Resistance_Lightning = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Lightning"),
		FString("Lightning damage resistance")
	);

	GameplayTags.Attributes_Resistance_Arcane = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Arcane"),
		FString("Arcane damage resistance")
	);

	GameplayTags.Attributes_Resistance_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Resistance.Physical"),
		FString("Physical damage resistance")
	);

	/*
	Map of Damage types to Resistances
	*/
	GameplayTags.DamageTypesToResistances.Add(GameplayTags.Damage_Fire, GameplayTags.Attributes_Resistance_Fire);
	GameplayTags.DamageTypesToResistances.Add(GameplayTags.Damage_Lightning, GameplayTags.Attributes_Resistance_Lightning);
	GameplayTags.DamageTypesToResistances.Add(GameplayTags.Damage_Arcane, GameplayTags.Attributes_Resistance_Arcane);
	GameplayTags.DamageTypesToResistances.Add(GameplayTags.Damage_Physical, GameplayTags.Attributes_Resistance_Physical);
	
	/*
	Resistances
	*/

	GameplayTags.Debuff_Burn = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Burn"),
		FString("Fire damage resistance")
	);

	GameplayTags.Debuff_Stun = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Stun"),
		FString("Lightning damage resistance")
	);
	
	GameplayTags.Debuff_Chance = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Chance"),
		FString("Debuff Chance")
	);

	GameplayTags.Debuff_Duration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Duration"),
		FString("Debuff Duration")
	);
	
	GameplayTags.Debuff_Frequency = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Frequency"),
		FString("Debuff Frequency")
	);

	GameplayTags.Debuff_Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Debuff.Damage"),
		FString("Debuff Damage")
	);
	

	// (Debuffs are no longer derived from damage type - each ability lists its own
	// FAuraDebuffSpec entries. Debuff.Chance/Duration/Frequency/Damage stay registered
	// as tags but are no longer used as SetByCaller magnitudes.)

	/*
	Effects
	*/

	GameplayTags.HitReact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("HitReact"),
		FString("Tag granted when Hit Reacting")
	);

	GameplayTags.Cooldown = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown"),
		FString("Tag granted when ability is used")
	);
	
	GameplayTags.Knockback = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Knockback"),
		FString("Tag granted when Knockback is used")
	);
	
	GameplayTags.DeathImpulse = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("DeathImpulse"),
		FString("Tag granted when DeathImpulse is used")
	);

	/*
	Abilities
	*/
	
	GameplayTags.Abilities_None= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.None"),
		FString("Empty Ability Tag")
	);

	GameplayTags.Abilities_Attack = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Attack"),
		FString("Attack Ability Tag")
	);

	GameplayTags.Abilities_Summon= UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Summon"),
		FString("Summon Ability Tag")
	);

	GameplayTags.Abilities_Fire_FireBolt = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Fire.FireBolt"),
		FString("FireBolt Ability Tag")
	);

	GameplayTags.Abilities_Fire_Meteorite = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Fire.Meteorite"),
		FString("Meteorite Ability Tag")
	);

	GameplayTags.Abilities_Lightning_Electrocute = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Lightning.Electrocute"),
		FString("Electrocute Ability Tag")
	);
	
	GameplayTags.Abilities_Passive_AuraBuff = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Passive.AuraBuff"),
		FString("AuraBuff Ability Tag")
	);
	
	GameplayTags.AbilityType_Passive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("AbilityType.Passive"),
		FString("Passive Ability Tag")
	);
	
	GameplayTags.AbilityType_Offensive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("AbilityType.Offensive"),
		FString("Offensive Ability Tag")
	);
	
	
	GameplayTags.Status_Eligible = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Eligible"),
		FString("Eligible Ability Tag")
	);
	
	GameplayTags.Status_Equipped = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Equipped"),
		FString("Equipped Ability Tag")
	);
	
	GameplayTags.Status_Unlocked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Unlocked"),
		FString("Unlocked Ability Tag")
	);
	
	GameplayTags.Status_Locked = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Status.Locked"),
		FString("Locked Ability Tag")
	);


	/*
	Cooldown Tags
	*/
	GameplayTags.Cooldown_Fire_FireBolt = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown.Fire.FireBolt"),
		FString("FireBolt Ability Cooldown Tag")
	);

	GameplayTags.Cooldown_Fire_Meteorite = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown.Fire.Meteorite"),
		FString("Meteorite Ability Cooldown Tag")
	);

	GameplayTags.Cooldown_Lightning_Electrocute = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Cooldown.Lightning.Electrocute"),
		FString("Electrocute Ability Cooldown Tag")
	);

	/*
	Combat Sockets
	*/

	GameplayTags.CombatSocket_Weapon = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CombatSocket.Weapon"),
		FString("Weapon")
	);

	GameplayTags.CombatSocket_RightHand = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CombatSocket.RightHand"),
		FString("Right Hand")
	);

	GameplayTags.CombatSocket_LeftHand = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CombatSocket.LeftHand"),
		FString("Left Hand")
	);

	/*
	Montage Tags
	*/

	GameplayTags.Montage_Attack_1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.1"),
		FString("Attack 1")
	);

	GameplayTags.Montage_Attack_2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.2"),
		FString("Attack 2")
	);

	GameplayTags.Montage_Attack_3 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.3"),
		FString("Attack 3")
	);

	GameplayTags.Montage_Attack_4 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Montage.Attack.4"),
		FString("Attack 4")
	);

	/* Character Tags */
	GameplayTags.Enemy = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Enemy"),
		FString("Enemy Tag")
	);

	GameplayTags.Aura = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Aura"),
		FString("Aura Tag")
	);
	
	GameplayTags.Effect_Status_Applied = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Effect.Status.Applied"),
		FString("Effect Status Applied")
	);

	/*
	Gameplay Cue Tags
	*/
	GameplayTags.GameplayCue_MeleeImpact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.MeleeImpact"),
		FString("Melee Impact Cue")
	);

	GameplayTags.GameplayCue_ShockBurst = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.ShockBurst"),
		FString("One-shot electric impact Cue")
	);

	GameplayTags.GameplayCue_ShockLoop = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.ShockLoop"),
		FString("Looping electric arc Cue")
	);

	// Child of ShockLoop with no notify of its own, so the cue manager routes it to
	// GC_ShockLoop. The cue tells the two roles apart via Parameters.OriginalTag -
	// NOT MatchedTagName, which is the handler's tag (GameplayCue.ShockLoop) for both.
	GameplayTags.GameplayCue_ShockLoop_Fork = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("GameplayCue.ShockLoop.Fork"),
		FString("Looping electric arc from the primary target to a forked target")
	);
}
