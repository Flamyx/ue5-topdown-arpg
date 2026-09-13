// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/AuraDamageGameplayAbility.h"
#include "Actor/MeteoriteProjectile.h"
#include "MeteoriteProjectileSpell.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMeteoriteProjectileSpell : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
							const FGameplayAbilityActorInfo* ActorInfo,
							const FGameplayAbilityActivationInfo ActivationInfo) override;


protected:
	UFUNCTION(BlueprintCallable)
	void SpawnProjectile(const FVector& ProjectileTargetLocation);

	UFUNCTION(BlueprintCallable)
	void StartChargeTimeline();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void CircleActorTick();

	void ChargeTick();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bChargeEnded = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AActor> RedCircleActor;

	// Writable from BP so the WaitInputRelease-based graph can feed TimeHeld straight
	// in before calling SpawnProjectile (which clamps it to [0, MaxChargeTime])
	UPROPERTY(BlueprintReadWrite, Category = "Charge")
	float ChargeTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charge")
	float MaxChargeTime = 3.f;

private:
	FTimerHandle ChargeTimerHandle;

	UPROPERTY(EditAnywhere)
	FScalableFloat ChargedMultiplier;

	UPROPERTY(EditAnywhere)
	float SpawnHeight;

	UPROPERTY(EditAnywhere)
	FVector2D SpawnOffsetBounds;

	UPROPERTY(EditAnywhere)
	float FChargeTick = .2f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AMeteoriteProjectile> ProjectileClass;
};
