// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/GameplayAbility/AuraGameplayAbility.h"
#include "AuraGameplayTags.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/GameplayAbility/AuraDamageGameplayAbility.h"

void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
	OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::EffectApplied);

	/*const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	GEngine->AddOnScreenDebugMessage(
		-1,
		10.f,
		FColor::Orange,
		FString::Printf(TEXT("Tag: %s"), *GameplayTags.Attributes_Secondary_Armor.ToString())
	);*/
}

bool UAuraAbilitySystemComponent::CheckIsAbilityAdded(const FGameplayTag& InputTag, bool bClearIfExists)
{
	for (auto& AbilitySpec : GetActivatableAbilities())
	{
		auto AbilityTag = UAuraAbilitySystemLibrary::FindAbilityTagFromSpec(AbilitySpec);
		auto AbilityInputTag = UAuraAbilitySystemLibrary::FindInputTagFromAbilityInfo(this, AbilityTag);
		
		if (InputTag.MatchesTagExact(AbilityInputTag))
		{
			if (bClearIfExists)
			{
				ClearAbility(AbilitySpec.Handle);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Ability exists and clear");
			}
			return true;
		}
	}
	return false;
}

void UAuraAbilitySystemComponent::AddAbility(TSubclassOf<UGameplayAbility>& AbilityClass, int32 Level, bool bClearIfExists)
{
	FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, Level);
	auto AbilityTag = UAuraAbilitySystemLibrary::FindAbilityTagFromSpec(AbilitySpec);
	auto InputTag = UAuraAbilitySystemLibrary::FindInputTagFromAbilityInfo(this, AbilityTag);
	if (CheckIsAbilityAdded(InputTag, bClearIfExists) && !bClearIfExists)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Ability exists and NO clear");
		return;
	}
	
	const UAuraGameplayAbility* AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
	if (AuraAbility)
	{
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(UAuraAbilitySystemLibrary::FindStatusTagFromAbilityInfo(this, AbilityTag));
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityTag);
		GiveAbility(AbilitySpec);
	}
}

void UAuraAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
	for (auto AbilityClass : StartupAbilities)
	{
		AddAbility(AbilityClass, 1, true);
	}
	bStartupAbilitiesGiven = true;
	AbilitiesGiven.Broadcast();
}

void UAuraAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
	for (auto AbilityClass : StartupPassiveAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (auto& AbilitySpec : GetActivatableAbilities())
	{
		auto AbilityTags = AbilitySpec.GetDynamicSpecSourceTags();
		auto CheckTags = AbilitySpec.Ability.Get()->GetAssetTags();
		if (AbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);
			if (AbilitySpec.IsActive())
			{
				const FPredictionKey PredictionKey = UAuraAbilitySystemLibrary::GetPredictionKey(AbilitySpec);
				InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, PredictionKey);
			}
		}
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (auto& AbilitySpec : GetActivatableAbilities())
	{
		auto AbilityTags = AbilitySpec.GetDynamicSpecSourceTags();
		auto CheckTags = AbilitySpec.Ability.Get()->GetAssetTags();
		if (AbilityTags.HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(AbilitySpec);
			if (!AbilitySpec.IsActive())
			{
				TryActivateAbility(AbilitySpec.Handle);
			}
		}
	}
}

void UAuraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;

	for (auto& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) && AbilitySpec.IsActive())
		{
			AbilitySpecInputReleased(AbilitySpec);
			const FPredictionKey PredictionKey = UAuraAbilitySystemLibrary::GetPredictionKey(AbilitySpec);
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, PredictionKey);
		}
	}
}

void UAuraAbilitySystemComponent::ForEachAbility(const FForEachAbility& Delegate)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		//Delegate.Broadcast(AbilitySpec);
		if (!Delegate.ExecuteIfBound(AbilitySpec))
		 {
		 	UE_LOG(LogTemp, Error, TEXT("Failed to execute delegate in %hs"), __FUNCTION__);
		 }
	}
}

void UAuraAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	// <lambda int(int , int )> add = [](int a, int b) { return a + b; };
	// auto multiply = [](int a, int b) { return a * b; };

	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
		{
			ServerUpgradeAttribute(AttributeTag);
		}
	}
}

void UAuraAbilitySystemComponent::UpgradeSpell(const FGameplayTag& SpellTag)
{
	if (GetAvatarActor()->Implements<UPlayerInterface>())
	{
		if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
		{
			ServerUpgradeSpell(SpellTag);
		}
	}
}

void UAuraAbilitySystemComponent::ServerUpgradeSpell_Implementation(const FGameplayTag& SpellTag)
{
	if (auto AbilitySpec = GetSpecFromAbilityTag(SpellTag))
	{
		FGameplayTag Status = UAuraAbilitySystemLibrary::FindStatusTagFromSpec(*AbilitySpec);
		const FAuraGameplayTags Tags = FAuraGameplayTags::Get();
		if (Status.MatchesTagExact(Tags.Status_Eligible))
		{
			AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(Tags.Status_Eligible);
			AbilitySpec->GetDynamicSpecSourceTags().AddTag(Tags.Status_Unlocked);
			AbilitySpec->GetDynamicSpecSourceTags().AddTag(SpellTag);
			Status = Tags.Status_Unlocked;
		}
		else if (Status.MatchesTagExact(Tags.Status_Equipped) || Status.MatchesTagExact(Tags.Status_Unlocked))
		{
			AbilitySpec->Level += 1;
		}
		IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), -1);
		MarkAbilitySpecDirty(*AbilitySpec);
		ClientUpdateAbilityStatus(SpellTag, Status, AbilitySpec->Level);
	}
}

void UAuraAbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag& AttributeTag)
{
	FGameplayEventData Payload;
	Payload.EventMagnitude = 1.f;
	Payload.EventTag = AttributeTag;
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), AttributeTag, Payload);
	IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(), -1);
}

void UAuraAbilitySystemComponent::ServerEquipSpell_Implementation(const FGameplayTag& InputTag,
	const FGameplayTag& AbilityTag)
{
	auto AbilitySpec = GetSpecFromAbilityTag(AbilityTag);
	auto Status = UAuraAbilitySystemLibrary::FindStatusTagFromSpec(*AbilitySpec);
	auto PrevSlot = UAuraAbilitySystemLibrary::FindInputTagFromSpec(*AbilitySpec);
	
	FString text = FString::Printf(TEXT("Ability %s, Status %s, Prev Tag %s"), *AbilityTag.ToString(), *Status.ToString(), *PrevSlot.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, text);
	ClearAbilitiesOfSlot(InputTag);
	ClearSlot(AbilitySpec);
	
	AbilitySpec->GetDynamicSpecSourceTags().AddTag(InputTag);
	
	if (Status.MatchesTagExact(FAuraGameplayTags::Get().Status_Unlocked))
	{
		AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Status_Unlocked);
		Status = FGameplayTag::RequestGameplayTag("Status.Equipped");
		AbilitySpec->GetDynamicSpecSourceTags().AddTag(Status);
	}
	
	MarkAbilitySpecDirty(*AbilitySpec);
	ClientEquipSpell(PrevSlot, InputTag, AbilityTag, Status);
}

void UAuraAbilitySystemComponent::ClientEquipSpell_Implementation(const FGameplayTag& PrevSlot,
	const FGameplayTag& Slot, const FGameplayTag& AbilityTag, const FGameplayTag& Status)
{
	SpellEquippedDelegate.Broadcast(AbilityTag, Status, Slot, PrevSlot);
}

void UAuraAbilitySystemComponent::UpdateAbilities(int32 Level)
{
	auto AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
	for (int i = 0; i < AbilityInfo->AbilityInfos.Num(); ++i)
	{
		auto Info = AbilityInfo->AbilityInfos[i];
		auto AbilitySpec = GetSpecFromAbilityTag(Info.AbilityTag);
		if (!Info.AbilityTag.IsValid() || Level < Info.LevelRequirement)
			continue;
		if (AbilitySpec == nullptr)
		{
			//AddAbility(Info.Ability, 1);
			auto NewAbilitySpec = FGameplayAbilitySpec(Info.Ability, 1.f);
			auto InputTag = UAuraAbilitySystemLibrary::FindInputTagFromAbilityInfo(this, Info.AbilityTag);
			NewAbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);
			NewAbilitySpec.GetDynamicSpecSourceTags().AddTag(FGameplayTag::RequestGameplayTag("Status.Eligible"));
			NewAbilitySpec.GetDynamicSpecSourceTags().AddTag(Info.AbilityTag);
			GiveAbility(NewAbilitySpec);
			ClientUpdateAbilityStatus(Info.AbilityTag, FGameplayTag::RequestGameplayTag("Status.Eligible"), 1);
			
		}
	}
}

void UAuraAbilitySystemComponent::ClientUpdateAbilityStatus_Implementation(const FGameplayTag& AbilityTag,
	const FGameplayTag& StatusTag, int32 Level)
{
	StatusChangedDelegate.Broadcast(AbilityTag, StatusTag, Level);
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag& AbilityTag)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (auto& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.GetDynamicSpecSourceTags().HasTag(AbilityTag))
			return &AbilitySpec;
	}
	return nullptr;
}

bool UAuraAbilitySystemComponent::GetDescriptions(const FGameplayTag& AbilityTag, FString& Description, FString& NextDescription)
{
	const auto AbilitySpec = GetSpecFromAbilityTag(AbilityTag);
	if (AbilitySpec)
	{
		int32 AbilityLevel = AbilitySpec->Level;
		int32 Damage = 0;
		int32 NextLevelDamage = 0;
		if (auto AuraDagameGA = Cast<UAuraDamageGameplayAbility>(AbilitySpec->Ability))
		{
			Damage += AuraDagameGA->Damage.GetValueAtLevel(AbilityLevel);
			NextLevelDamage += AuraDagameGA->Damage.GetValueAtLevel(AbilityLevel + 1);
		}
		
		if (auto AuraGA = Cast<UAuraGameplayAbility>(AbilitySpec->Ability))
		{
			Description = AuraGA->GetDescription(AbilityLevel, Damage, AbilityTag.ToString());
			NextDescription = AuraGA->GetNextLevelDescription(AbilityLevel + 1, NextLevelDamage);
			return true;
		}	
	}
	Description = FString();
	NextDescription = FString();
	return false;
}

void UAuraAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	// Super bails out and retries itself 0.5s later while any spec's Ability hasn't resolved yet.
	// Broadcasting then hands the UI specs with no ability (FindAbilityTagFromSpec returns nothing),
	// and a one-shot flag would swallow the retry that carries the real data.
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.Ability) return;
	}

	// Every replicated change, not just the first: a later rep (equip, unlock, abilities granted
	// after the first update) must reach the overlay too. Re-broadcasting the same info is harmless.
	bStartupAbilitiesGiven = true;
	AbilitiesGiven.Broadcast();
}

void UAuraAbilitySystemComponent::ClearAbilitiesOfSlot(const FGameplayTag& SlotTag)
{
	FScopedAbilityListLock ActiveScopeLock(*this);
	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		bool bHasTag = false;
		for (auto Tag: AbilitySpec.GetDynamicSpecSourceTags())
		{
			if (Tag.MatchesTagExact(SlotTag))
			{
				bHasTag = true;
			}
		}
		if (bHasTag)
		{
			ClearSlot(&AbilitySpec);
		}
	}
}

void UAuraAbilitySystemComponent::ClearSlot(FGameplayAbilitySpec* AbilitySpec)
{
	auto Slot = UAuraAbilitySystemLibrary::FindInputTagFromSpec(*AbilitySpec);
	AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(Slot);
	//AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Status_Equipped);
	FString text = FString::Printf(TEXT("Removing Slot Ability %s, Prev Tag %s"), *UAuraAbilitySystemLibrary::FindAbilityTagFromSpec(*AbilitySpec).ToString(), *Slot.ToString());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, text);
	MarkAbilitySpecDirty(*AbilitySpec);
}

void UAuraAbilitySystemComponent::EffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	if (TagContainer.Num() > 0)
	{
		ClientEffectApplied(TagContainer);
	}
}

void UAuraAbilitySystemComponent::ClientEffectApplied_Implementation(const FGameplayTagContainer& AssetTags)
{
	EffectAssetTags.Broadcast(AssetTags);
}
 