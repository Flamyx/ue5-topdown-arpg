// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/AuraChanneledAbility.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "ElectrocuteCourse.generated.h"

/**
 * Channelled lightning beam. UAuraChanneledAbility provides the channel lock and the cursor aim
 * loop; this class retargets the primary on every aim sample, forks to nearby enemies, and deals
 * damage on its own, slower tick.
 */
UCLASS()
class AURA_API UElectrocuteCourse : public UAuraChanneledAbility
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void TraceFirstTarget(const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable)
	void StoreHitResult(const FHitResult& HitResult);

	UFUNCTION(BlueprintImplementableEvent)
	void PrimaryActorDied(AActor* DeadActor);

	UPROPERTY()
	TObjectPtr<AActor> BoundPrimary;

	void SetPrimaryTarget(AActor* NewTarget);
	void UnbindPrimary();

protected:
	/** Every aim sample: refine the cursor hit into the beam's first target */
	virtual void OnAimUpdated(const FHitResult& Hit) override;

	UPROPERTY(EditDefaultsOnly)
	float ForkRadius;

	// Holds the fork TARGETS, not their ASCs: the radius query returns actors, so
	// storing actors keeps every comparison a direct pointer compare. The ASC is
	// fetched on demand at the two places that need it (add/remove cue).
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<AActor>> AffectedActors;

	UFUNCTION(BlueprintCallable)
	void AffectActors();

	UFUNCTION(BlueprintCallable)
	void RemoveEffects();

	UFUNCTION(BlueprintCallable)
	void DamageActors();

	/** Starts the channel (aim loop included), the caster-hosted beam cue and the damage tick */
	UFUNCTION(BlueprintCallable)
	void RunDamageLogic();

	/** Seconds between damage ticks. Independent of the parent's AimTickRate. */
	UPROPERTY(EditDefaultsOnly)
	float FChargeTick;

	UPROPERTY(EditDefaultsOnly)
	float TargetGraceSeconds;

	double LastValidTargetTime = 0.0;

	/** Mana cost multiplier by beam count (curve X = primary + forks, 0 = channeling at nothing).
	 *  The Blueprint's CommitAbility pays the cost GE and starts the cooldown; every damage tick then
	 *  pays the cost again (cost only), and the channel ends when it can't. */
	UPROPERTY(EditDefaultsOnly, Category = "Cost")
	FScalableFloat BeamManaCostMultiplier = 1.f;

	virtual float GetManaCostMultiplier() const override;

	/** Primary target plus live forks */
	UFUNCTION(BlueprintPure)
	int32 GetBeamCount() const;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void DamageTick();
	void DamageTarget(AActor* TargetActor);

	FTimerHandle ChargeTimerHandle;
};
