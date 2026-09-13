// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Data/AbilityInfo.h"
#include "AuraAbilitySystemComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FEffectAssetTags, const FGameplayTagContainer& /* AssetTags */);
DECLARE_MULTICAST_DELEGATE(FAbilitiesGiven);
DECLARE_DELEGATE_OneParam(FForEachAbility, const FGameplayAbilitySpec&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FonStatusChanged, const FGameplayTag& /* AbilityTag */, const FGameplayTag& /* StatusTag */, int32 /* AbilityLevel */);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnSpellEquipped, const FGameplayTag& /* AbilityTag */, const FGameplayTag& /* StatusTag */, const FGameplayTag& /* PrevSlot */, const FGameplayTag& /* Slot */);

/**
 * 
 */
UCLASS()
class AURA_API UAuraAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	void AbilityActorInfoSet();

	FEffectAssetTags EffectAssetTags;
	FAbilitiesGiven AbilitiesGiven;
	FonStatusChanged StatusChangedDelegate;
	FOnSpellEquipped SpellEquippedDelegate;
	bool bStartupAbilitiesGiven = false;
	
	bool CheckIsAbilityAdded(const FGameplayTag& InputTag, bool bClearIfExists);
	void AddAbility(TSubclassOf<UGameplayAbility> &AbilityClass, int32 Level, bool bClearIfExists = false);
	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);
	void AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities);
	
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagHeld(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void ForEachAbility(const FForEachAbility& Delegate);
	
	//void UpdateAbility(const FForEachAbility& Delegate, );

	void UpgradeAttribute(const FGameplayTag& AttributeTag);
	UFUNCTION(Server, Reliable)
	void ServerUpgradeAttribute(const FGameplayTag& AttributeTag);

	void UpgradeSpell(const FGameplayTag& SpellTag);
	
	UFUNCTION(Server, Reliable)
	void ServerUpgradeSpell(const FGameplayTag& SpellTag);
	
	UFUNCTION(Server, Reliable)
	void ServerEquipSpell(const FGameplayTag &InputTag, const FGameplayTag &AbilityTag);
	
	UFUNCTION(Client, Reliable)
	void ClientEquipSpell(const FGameplayTag &PrevSlot, const FGameplayTag &Slot, const FGameplayTag &AbilityTag, const FGameplayTag &Status);
	
	void UpdateAbilities(int32 Level);
	
	UFUNCTION(Client, Reliable)
	void ClientUpdateAbilityStatus(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 Level);
	
	FGameplayAbilitySpec* GetSpecFromAbilityTag(const FGameplayTag& AbilityTag);

	bool GetDescriptions(const FGameplayTag& AbilityTag, FString& Description, FString& NextDescription);
	
protected:
	virtual void OnRep_ActivateAbilities();

	void EffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle);

	// Only the asset tags cross the wire — the full FGameplayEffectSpec is expensive
	// to serialize and was sent on every single effect application
	UFUNCTION(Client, Reliable)
	void ClientEffectApplied(const FGameplayTagContainer& AssetTags);

private:
	void ClearAbilitiesOfSlot(const FGameplayTag& SlotTag);
	void ClearSlot(FGameplayAbilitySpec* AbilitySpec);
};
