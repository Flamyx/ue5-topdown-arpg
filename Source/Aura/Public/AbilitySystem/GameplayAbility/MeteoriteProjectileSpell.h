// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/AuraChanneledAbility.h"
#include "Actor/MeteoriteProjectile.h"
#include "MeteoriteProjectileSpell.generated.h"

/**
 * Hold to charge, release to drop a meteor. UAuraChanneledAbility provides the channel lock, the
 * aim loop and caster facing while charging; charge time comes from the Blueprint's
 * WaitInputRelease TimeHeld and scales damage, blast radius and the projectile's visuals.
 */
UCLASS()
class AURA_API UMeteoriteProjectileSpell : public UAuraChanneledAbility
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable)
	void SpawnProjectile(const FVector& ProjectileTargetLocation);

	// Writable from BP so the WaitInputRelease-based graph can feed TimeHeld straight
	// in before calling SpawnProjectile (which clamps it to [0, MaxChargeTime])
	UPROPERTY(BlueprintReadWrite, Category = "Charge")
	float ChargeTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Charge")
	float MaxChargeTime = 3.f;

	/** Cost GE's base mana * ChargedManaCostMultiplier at the clamped ChargeTime. The Blueprint sets
	 *  ChargeTime before CommitAbility, so the commit pays for the charge actually released. */
	virtual float GetManaCostMultiplier() const override;

	virtual void CommitExecute(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

private:
	float GetEffectiveCharge() const { return FMath::Clamp(ChargeTime, 0.f, MaxChargeTime); }

	UPROPERTY(EditAnywhere)
	FScalableFloat ChargedMultiplier;

	/** Mana cost multiplier by charge seconds (curve X = seconds held, clamped to MaxChargeTime) */
	UPROPERTY(EditAnywhere, Category = "Charge")
	FScalableFloat ChargedManaCostMultiplier = 1.f;

	// A fully charged release can cost more than the mana that let the ability activate. The
	// Blueprint ignores CommitAbility's result, so SpawnProjectile checks this instead.
	bool bCommitted = false;

	UPROPERTY(EditAnywhere)
	float SpawnHeight;

	UPROPERTY(EditAnywhere)
	FVector2D SpawnOffsetBounds;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AMeteoriteProjectile> ProjectileClass;
};
