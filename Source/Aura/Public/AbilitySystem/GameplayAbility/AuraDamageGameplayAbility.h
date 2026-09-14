// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/AuraGameplayAbility.h"
#include "AuraAbilityTypes.h"
#include "AuraDamageGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraDamageGameplayAbility : public UAuraGameplayAbility
{
	GENERATED_BODY()

public:
	UAuraDamageGameplayAbility();

	UFUNCTION(BlueprintCallable)
	void CauseDamage(AActor* TargetActor);
	UFUNCTION(BlueprintPure)
	FAuraDamageEffectParams MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor = nullptr) const;
	
	UFUNCTION(BlueprintCallable)
	void StoreOwnerVariables();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FGameplayTag DamageType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FScalableFloat Damage;
	
	/** Debuffs this ability can apply on hit. Each is rolled independently against the target's
	 *  resistance to DamageType; a landed Debuff.Stun is also what plays the target's HitReact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TArray<FAuraDebuffSpec> Debuffs;
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	float DeathImpulseMagnitude = 1.2f;
	UPROPERTY(EditDefaultsOnly, Category = "Knockback")
	float KnockbackImpulseMagnitude = .2f;
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	FVector DeathImpulse = FVector::ZeroVector;
	UPROPERTY(EditDefaultsOnly, Category = "Knockback")
	FVector KnockbackImpulse = FVector::ZeroVector;
	
protected: 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> ResidualDamageEffectClass;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<ACharacter> OwnerCharacter;
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> OwnerPlayerController;
};
