// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/AuraDamageGameplayAbility.h"
#include "AuraChanneledAbility.generated.h"

class UTargetDataUnderMouse;

/**
 * Base for hold-to-channel abilities (Electrocute, Meteorite). Owns what every channel needs:
 *  - StartChannel / EndChannel: lock the caster in place, toggle the channel pose, and turn the
 *    caster's per-frame aim smoothing and facing on and off (see AAuraCharacterBase::Tick).
 *  - An aim loop that re-samples the cursor every AimTickRate seconds while channeling, updates
 *    MouseHitLocation/MouseHitActor, and pushes the aim point to the caster.
 *  - Teardown in EndAbility, so release, cancel, interrupt and death all undo the channel.
 */
UCLASS(Abstract)
class AURA_API UAuraChanneledAbility : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

public:
	/** Lock movement, enable the channel pose and facing, and start the aim loop. Safe to call twice. */
	UFUNCTION(BlueprintCallable, Category = "Channel")
	void StartChannel();

	/** Undo StartChannel. Idempotent; EndAbility always calls it. */
	UFUNCTION(BlueprintCallable, Category = "Channel")
	void EndChannel();

protected:
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Called for every aim sample that hit something, after MouseHitLocation/Actor are updated.
	 *  Runs synchronously on the owning client and whenever the sample arrives on the server. */
	virtual void OnAimUpdated(const FHitResult& Hit) {}

	bool IsChanneling() const { return bChanneling; }

	void SetCasterAimLocation(const FVector& Location) const;

	UPROPERTY(BlueprintReadWrite)
	FVector MouseHitLocation;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> MouseHitActor;

	/** Seconds between cursor samples while channeling. Each sample is a reliable target-data RPC,
	 *  so don't go much below ~0.05; the caster smooths between samples every frame anyway. */
	UPROPERTY(EditDefaultsOnly, Category = "Channel", meta = (ClampMin = "0.01"))
	float AimTickRate = 0.05f;

private:
	void AimTick();
	void RequestCursorTarget();

	UFUNCTION()
	void OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data);

	UPROPERTY()
	TObjectPtr<UTargetDataUnderMouse> TargetDataTask;

	FTimerHandle AimTimerHandle;
	bool bChanneling = false;
};
