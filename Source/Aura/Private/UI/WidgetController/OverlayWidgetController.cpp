// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Player/AuraPlayerState.h"

void UOverlayWidgetController::BroadcastInitialValues()
{
	OnHealthChanged.Broadcast(GetAuraAS()->GetHealth());
	OnMaxHealthChanged.Broadcast(GetAuraAS()->GetMaxHealth());
	OnManaChanged.Broadcast(GetAuraAS()->GetMana());
	OnMaxManaChanged.Broadcast(GetAuraAS()->GetMaxMana());

	// Re-push the ability icons now that the widget is bound (same as the spell menu
	// controller does). BindCallbacksToDependencies runs inside AAuraHUD's widget
	// controller getter - a line BEFORE SetWidgetController - so on a client, where the
	// abilities have already replicated in and bStartupAbilitiesGiven is true, its
	// immediate BroadcastAbilityInfo() reaches no listeners and the globes stay empty.
	// No-op on the server, where the abilities are granted after the overlay is built.
	BroadcastAbilityInfo();
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		GetAuraAS()->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data) {
				OnHealthChanged.Broadcast(Data.NewValue);}
		);
	

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		GetAuraAS()->GetMaxHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data) {
				OnMaxHealthChanged.Broadcast(Data.NewValue);}
		);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		GetAuraAS()->GetManaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data) {
				OnManaChanged.Broadcast(Data.NewValue);}
		);
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		GetAuraAS()->GetMaxManaAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data) {
				OnMaxManaChanged.Broadcast(Data.NewValue);}
		);

	/* Example of a Callback way instead of Lambda
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		GetAuraAS()->GetMaxManaAttribute()).AddUObject(this, &UOverlayWidgetController::MaxManaChanged);*/

	if (GetAuraASC())
	{
		// Always bind: on a client AbilitiesGiven fires on every replicated ability change, so the
		// globes follow later reps even when the first one landed before this controller existed
		AuraASC->AbilitiesGiven.AddUObject(this, &UOverlayWidgetController::BroadcastAbilityInfo);
		if (AuraASC->bStartupAbilitiesGiven)
			BroadcastAbilityInfo();

		AuraASC->EffectAssetTags.AddLambda(
			[this](const FGameplayTagContainer& AssetTags)
			{
				for (const FGameplayTag& Tag : AssetTags)
				{
					FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
					if (Tag.MatchesTag(MessageTag))
					{
						FUIWidgetRow* Row = GetDataTableByRow<FUIWidgetRow>(MessageWidgetDataTable, Tag);
						MessageWidgetRowDelegate.Broadcast(*Row);
					}
					/*const FString Msg = FString::Printf(TEXT("GE Tag: %s"), *Tag.ToString());
					GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Blue, Msg);*/
				}
			}
		);
		
		AuraASC->SpellEquippedDelegate.AddUObject(this, &UOverlayWidgetController::SpellEquipped);
		
		GetAuraASC()->StatusChangedDelegate.AddLambda(
		[this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 Level)
		{
			auto AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAuraASC());
			auto AbilityCfg = AbilityInfo->GetAbilityInfo(AbilityTag);
			auto AbilitySpec = GetAuraASC()->GetSpecFromAbilityTag(AbilityTag);
			if (AbilitySpec)
			{
				AbilityCfg.StatusTag = StatusTag;
				AbilityCfg.InputTag = UAuraAbilitySystemLibrary::FindInputTagFromSpec(*AbilitySpec);
			}
			AbilityCfgDelegate.Broadcast(AbilityCfg);
		}
		);
	}

	GetAuraPS()->XPDelegate.AddUObject(this, &UOverlayWidgetController::OnXPChanged);
	GetAuraPS()->LevelDelegate.AddLambda(
		[this](int32 NewLevel)
		{
			OnPlayerLevelChangedDelegate.Broadcast(NewLevel);
		}
	);
	
}

void UOverlayWidgetController::SpellEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status,
	const FGameplayTag& Slot, const FGameplayTag& PrevSlot)
{
	FAuraAbilityInfo PrevSlotInfo;
	PrevSlotInfo.AbilityTag = FAuraGameplayTags::Get().Abilities_None;
	PrevSlotInfo.InputTag = PrevSlot;
	PrevSlotInfo.StatusTag = FAuraGameplayTags::Get().Status_Unlocked;
	
	AbilityCfgDelegate.Broadcast(PrevSlotInfo);
	
	FAuraAbilityInfo SlotInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAuraASC())->GetAbilityInfo(AbilityTag);
	SlotInfo.InputTag = Slot;
	SlotInfo.StatusTag = Status;
	AbilityCfgDelegate.Broadcast(SlotInfo);
}

void UOverlayWidgetController::MaxManaChanged(const FOnAttributeChangeData& Data) const
{
	OnMaxManaChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnXPChanged(int32 newXP)
{
	int32 PlayerLevel = GetAuraPS()->FindLevelForXP(newXP);

	auto CurrentLevelInfo = GetAuraPS()->GetLevelUpInfo(PlayerLevel);
	auto NextLevelInfo = GetAuraPS()->GetLevelUpInfo(PlayerLevel + 1);
	if (!CurrentLevelInfo.LevelUpRequirement || !NextLevelInfo.LevelUpRequirement)
		return;

	float PercentageXP = static_cast<float>(newXP - CurrentLevelInfo.LevelUpRequirement) / static_cast<float>(NextLevelInfo.LevelUpRequirement - CurrentLevelInfo.LevelUpRequirement);
	OnXPChangedDelegate.Broadcast(PercentageXP);
}


void UOverlayWidgetController::OnLvlUp(const FLevelUpInfo& LvlUpInfo)
{
}
