// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "AuraGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FGameplayTag StartupInputTag;

	virtual FString GetDescription(int32 Level, float Damage, const FString& Title);
	virtual FString GetNextLevelDescription(int32 Level, float Damage);
	virtual FString GetLockedDescription(int32 Level);
	
protected:
	float GetManaCost(float Level) const;
	float GetCooldown(float Level) const;
	
	UPROPERTY(EditAnywhere)
	int32 MaxAbilityLevel = 25;
};
