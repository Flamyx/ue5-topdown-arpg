// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Overlay/AuraHud.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "Game/AuraGameModeBase.h"
#include "Character/AuraEnemy.h"
#include "AbilitySystemComponent.h"
#include "Player/AuraPlayerState.h"
#include "Engine/OverlapResult.h"
#include "AuraAbilityTypes.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "Engine/Engine.h"
#include "Game/AuraGameStateBase.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/GameModeBase.h"

#if ENABLE_DRAW_DEBUG
static TAutoConsoleVariable<int32> CVarAuraShowRadiusDebug(
	TEXT("Aura.ShowRadiusDebug"),
	0,
	TEXT("Draw the query sphere used by GetLivePlayersWithinRadius.\n")
	TEXT("0: off (default)\n")
	TEXT("1: on"),
	ECVF_Cheat);
#endif

UOverlayWidgetController* UAuraAbilitySystemLibrary::GetOverlayWidgetContoller(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(PC->GetHUD()))
		{
			AAuraPlayerState* PS = PC->GetPlayerState<AAuraPlayerState>();
			UAttributeSet* AS = PS->GetAttributeSet();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();

			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			UOverlayWidgetController* OWC = AuraHUD->GetOverlayWidgetController(WidgetControllerParams);
			return OWC;
		}
	}

	return nullptr;
}

UAttributeMenuWidgetController* UAuraAbilitySystemLibrary::GetAttributeMenuWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(PC->GetHUD()))
		{
			AAuraPlayerState* PS = PC->GetPlayerState<AAuraPlayerState>();
			UAttributeSet* AS = PS->GetAttributeSet();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();

			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			UAttributeMenuWidgetController* OWC = AuraHUD->GetAttributeMenuWidgetController(WidgetControllerParams);
			return OWC;
		}
	}

	return nullptr;
}

USpellMenuWidgetController* UAuraAbilitySystemLibrary::GetSpellMenuWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(PC->GetHUD()))
		{
			AAuraPlayerState* PS = PC->GetPlayerState<AAuraPlayerState>();
			UAttributeSet* AS = PS->GetAttributeSet();
			UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();

			const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
			USpellMenuWidgetController* SWC = AuraHUD->GetSpellMenuWidgetController(WidgetControllerParams);
			return SWC;
		}
	}
	return nullptr;
}

bool UAuraAbilitySystemLibrary::UpdateOverlay(const UObject* WorldContextObject)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(PC->GetHUD()))
		{
			AuraHUD->UpdateOverlay();
			return true;
		}
	}
	return false;
}

FGameplayTag UAuraAbilitySystemLibrary::FindInputTagFromAbilityInfo(const UObject* WorldContextObject, const FGameplayTag& AbilityTag)
{
	// Routed through GetAbilityInfo so this survives the client-side window where the
	// GameState has not replicated in yet (see the comment there)
	UAbilityInfo* AbilityInfo = GetAbilityInfo(WorldContextObject);
	if (AbilityInfo == nullptr) return FGameplayTag();
	return AbilityInfo->GetAbilityInfo(AbilityTag).InputTag;
}

FGameplayTag UAuraAbilitySystemLibrary::FindStatusTagFromAbilityInfo(const UObject* WorldContextObject,
	const FGameplayTag& AbilityTag)
{
	UAbilityInfo* AbilityInfo = GetAbilityInfo(WorldContextObject);
	if (AbilityInfo == nullptr) return FGameplayTag();
	return AbilityInfo->GetAbilityInfo(AbilityTag).StatusTag;
}

void UAuraAbilitySystemLibrary::InitializeEnemyAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	auto CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	FCharacterClassDefaultInfo CharacterInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);

	auto PrimaryAttributesContextHandle = ASC->MakeEffectContext();
	PrimaryAttributesContextHandle.AddSourceObject(ASC->GetAvatarActor());
	const FGameplayEffectSpecHandle PrimaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterInfo.PrimaryAttributes, Level, PrimaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributesSpecHandle.Data.Get());
	
	auto SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(ASC->GetAvatarActor());
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, Level, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	auto VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(ASC->GetAvatarActor());
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes, Level, VitalAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());
}

void UAuraAbilitySystemLibrary::GiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, ECharacterClass CharacterClass)
{
	auto CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) return;

	for (auto AbilityClass : CharacterClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
		
	}

	const FCharacterClassDefaultInfo DefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	for (auto AbilityClass : DefaultInfo.StartupAbilities)
	{
		if (ASC->GetAvatarActor()->Implements<UCombatInterface>())
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, ICombatInterface::Execute_GetPlayerLevel(ASC->GetAvatarActor()));
			ASC->GiveAbility(AbilitySpec);
		}
	}
}

float UAuraAbilitySystemLibrary::GetXPReward(const UObject* WorldContextObject, ECharacterClass CharacterClass, int32 CharacterLevel)
{
	auto CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr) 
		return 0;

	auto Info = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	const float XPReward = Info.XPReward.GetValueAtLevel(CharacterLevel);

	return XPReward;
}

FGameplayEffectContextHandle UAuraAbilitySystemLibrary::ApplyDamageEffect(FAuraDamageEffectParams DamageEffectParams)
{
	FGameplayEffectContextHandle ContextHandle = DamageEffectParams.SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(DamageEffectParams.SourceASC->GetAvatarActor());
	FGameplayEffectSpecHandle SpecHandle = DamageEffectParams.SourceASC->MakeOutgoingSpec(DamageEffectParams.DamageEffectClass, DamageEffectParams.AbilityLevel, ContextHandle);
	
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DamageEffectParams.DamageType, DamageEffectParams.Damage);
	// Debuffs ride in the context instead of as SetByCaller floats: an ability can carry several,
	// each with its own chance/duration/frequency/damage. The spec shares this context, so
	// UExecCalc_Damage sees them when the effect executes.
	if (FAuraGameplayEffectContext* AuraContext = FAuraGameplayEffectContext::ExtractEffectContext(ContextHandle))
	{
		AuraContext->SetPendingDebuffs(DamageEffectParams.Debuffs);
	}
	SetDeathImpulse(ContextHandle, DamageEffectParams.DeathImpulse);
	SetKnockbackImpulse(ContextHandle, DamageEffectParams.KnockbackImpulse);
	
	DamageEffectParams.TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	
	return ContextHandle;
}

TArray<FRotator> UAuraAbilitySystemLibrary::GetEvenlyScacedRotators(const FVector& Forward, const FVector& Axis,
	float Spread, int NumProjectiles)
{
	TArray<FRotator> Ret;
	// A single projectile flies straight at the target — the old math returned
	// LeftOfSpread for i=0, so a level-1 bolt veered off by Spread/2
	if (NumProjectiles <= 1)
	{
		Ret.Add(Forward.Rotation());
		return Ret;
	}

	// Step by Spread/(N-1) so the first and last rotators sit exactly on both edges
	// of the spread (the old Spread/N step compressed and skewed the fan)
	const FVector LeftOfSpread = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	const float DeltaSpread = Spread / (NumProjectiles - 1);
	Ret.Reserve(NumProjectiles);
	for (int i = 0; i < NumProjectiles; ++i)
	{
		Ret.Add(LeftOfSpread.RotateAngleAxis(DeltaSpread * i, Axis).Rotation());
	}

	return Ret;
}

FPredictionKey UAuraAbilitySystemLibrary::GetPredictionKey(const FGameplayAbilitySpec& Spec)
{
	TArray<UGameplayAbility*> AbilityInstances = Spec.GetAbilityInstances();

	// 2. Loop through instances to find a valid running instance
	for (UGameplayAbility* Instance : AbilityInstances)
	{
		if (Instance && Instance->IsActive())
		{
			// 3. Extract the key safely using the modern instance API
			FGameplayAbilityActivationInfo ActivationInfo = Instance->GetCurrentActivationInfo();
			return ActivationInfo.GetActivationPredictionKey();
		}
	}

	// 4. Fallback if no instances are active or the ability is not currently executing
	return FPredictionKey();
}

UBehaviorTree* UAuraAbilitySystemLibrary::GetBehaviorTree(const UObject* WorldContextObject, ECharacterClass CharacterClass)
{
	auto CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	const FCharacterClassDefaultInfo DefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	UBehaviorTree* CharacterClassBT = DefaultInfo.BehaviorTree;
	return CharacterClassBT;
}

UCharacterClassInfo* UAuraAbilitySystemLibrary::GetCharacterClassInfo(const UObject* WorldContextObject)
{
	const AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (AuraGameMode == nullptr) return nullptr;
	auto CharacterClassInfo = AuraGameMode->CharacterClassInfo;
	return CharacterClassInfo;
}

UAbilityInfo* UAuraAbilitySystemLibrary::GetAbilityInfo(const UObject* WorldContextObject)
{
	if (const AAuraGameStateBase* AuraGameState = Cast<AAuraGameStateBase>(UGameplayStatics::GetGameState(WorldContextObject)))
	{
		return AuraGameState->AbilityInfo;
	}

	// The GameState is a replicated actor, and on a client the ASC's ActivatableAbilities
	// OnRep can beat its arrival: UWorld::GetGameState() is still null when the widget
	// controllers ask for ability info, so the one-shot BroadcastAbilityInfo is lost and
	// the client's spell globes stay empty (NetMode=3, GameState=None).
	// AbilityInfo is static config that lives on the GameState *class default*, so fall
	// back to the CDO of the GameState class the level's GameMode declares - that is
	// available on every machine from level load, before any replication happens.
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World == nullptr) return nullptr;

	const AWorldSettings* WorldSettings = World->GetWorldSettings();
	// Null when the map inherits the project's GlobalDefaultGameMode instead of setting
	// a GameMode Override; every Aura map sets one in World Settings.
	if (WorldSettings == nullptr || !WorldSettings->DefaultGameMode) return nullptr;

	const AGameModeBase* GameModeCDO = WorldSettings->DefaultGameMode->GetDefaultObject<AGameModeBase>();
	if (GameModeCDO == nullptr || !GameModeCDO->GameStateClass) return nullptr;

	const AAuraGameStateBase* GameStateCDO = Cast<AAuraGameStateBase>(GameModeCDO->GameStateClass->GetDefaultObject());
	return GameStateCDO ? GameStateCDO->AbilityInfo : nullptr;
}

bool UAuraAbilitySystemLibrary::IsBlockedHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->IsBlockedHit();
	}
	return false;
}

bool UAuraAbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->IsCriticalHit();
	}
	return false;
}

bool UAuraAbilitySystemLibrary::IsDebuffHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->IsDebuffHit();
	}
	return false;
}

bool UAuraAbilitySystemLibrary::IsSuccessfulDebuff(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->IsSuccessfulDebuff();
	}
	return false;
}

TArray<FAuraDebuffSpec> UAuraAbilitySystemLibrary::GetSuccessfulDebuffs(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->GetSuccessfulDebuffs();
	}
	return TArray<FAuraDebuffSpec>();
}

FGameplayTag UAuraAbilitySystemLibrary::GetDamageType(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->GetDamageType();
	}
	
	return FGameplayTag();
}

FVector UAuraAbilitySystemLibrary::GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->GetDeathImpulse();
	}
	return FVector::ZeroVector;
}

FVector UAuraAbilitySystemLibrary::GetKnockbackImpulse(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		return AuraEffectContext->GetKnockbackImpulse();
	}
	return FVector::ZeroVector;
}

void UAuraAbilitySystemLibrary::SetDamageType(FGameplayEffectContextHandle& EffectContextHandle,
	const FGameplayTag& DamageType)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		TSharedPtr<FGameplayTag> InDamageType = MakeShared<FGameplayTag>(DamageType);
		AuraEffectContext->SetDamageType(InDamageType);
	}
}

void UAuraAbilitySystemLibrary::SetDeathImpulse(FGameplayEffectContextHandle& EffectContextHandle,
	const FVector& InImpulse)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		AuraEffectContext->SetDeathImpulse(InImpulse);
	}
}

void UAuraAbilitySystemLibrary::SetKnockbackImpulse(FGameplayEffectContextHandle& EffectContextHandle,
	const FVector& InImpulse)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		AuraEffectContext->SetKnockbackImpulse(InImpulse);
	}
}


void UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(const UObject* WorldContextObject, TArray<AActor*>& OutOverlappingActors, const TArray<AActor*>& ActorsToIgnore, float Radius, const FVector& SphereOrigin)
{
	FCollisionQueryParams SphereParams;
	SphereParams.AddIgnoredActors(ActorsToIgnore);

	const AActor* SourceActor = Cast<AActor>(WorldContextObject);
	AActor* MutableSourceActor = const_cast<AActor*>(SourceActor);
	
	// query scene to see what we hit
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		//DrawDebugSphere(World, SphereOrigin, Radius, 16, FColor::Green, false, 3.f, 0, 1.f);

		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, SphereOrigin, FQuat::Identity, 
			FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), 
			FCollisionShape::MakeSphere(Radius), SphereParams);
		for (FOverlapResult& Overlap : Overlaps)
		{
			AActor* Actor = Overlap.GetActor();
			if (Actor->Implements<UCombatInterface>() && !ICombatInterface::Execute_IsDead(Actor) && UAuraAbilitySystemLibrary::IsNotFriend(MutableSourceActor, Actor))
			{
				auto AuraCharacter = Cast<AAuraCharacterBase>(Actor);
				
				// Can simply use Actor, but with GetAvatar it's ok as well
				OutOverlappingActors.AddUnique(ICombatInterface::Execute_GetAvatar(Actor));
			}
		}
	}
}

bool UAuraAbilitySystemLibrary::IsNotFriend(AActor* FirstActor, AActor* SecondActor)
{
	if (!IsValid(FirstActor) || !IsValid(SecondActor)) return false;
	const bool bFirstIsPlayer = FirstActor->ActorHasTag(FName("Player"));
	const bool bSecondIsPlayer = SecondActor->ActorHasTag(FName("Player"));
	const bool bFriendly = bFirstIsPlayer == bSecondIsPlayer;
	return !bFriendly;
}

FGameplayTag UAuraAbilitySystemLibrary::FindAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability)
	{
		for (auto Tag : AbilitySpec.Ability.Get()->GetAssetTags())
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities"))))
				return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemLibrary::FindInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	if (AbilitySpec.Ability) 
	{
		for (auto Tag : AbilitySpec.GetDynamicSpecSourceTags())
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("InputTag"))))
				return Tag;
		}
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemLibrary::FindStatusTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (auto StatusTag: AbilitySpec.GetDynamicSpecSourceTags())
	{
		if (StatusTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Status"))))
			return StatusTag;
	}
	return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemLibrary::FindAbilityTypeTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
	for (auto TypeTag: AbilitySpec.GetDynamicSpecSourceTags())
	{
		if (TypeTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("AbilityType"))))
			return TypeTag;
	}
	return FGameplayTag();
}

void UAuraAbilitySystemLibrary::SetIsBlockedHit(FGameplayEffectContextHandle& EffectContextHandle, bool bInIsBlockedHit)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		AuraEffectContext->SetIsBlockedHit(bInIsBlockedHit);
	}
}

void UAuraAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& EffectContextHandle, bool bInIsCriticalHit)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = FAuraGameplayEffectContext::ExtractEffectContext(EffectContextHandle))
	{
		AuraEffectContext->SetIsCriticalHit(bInIsCriticalHit);
	}
}
