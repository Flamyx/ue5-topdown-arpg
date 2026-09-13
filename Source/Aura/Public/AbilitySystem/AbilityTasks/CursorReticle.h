// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "CursorReticle.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCursorReticleSpawnedSignature, AActor*, Reticle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCursorReticleEffectSpawnedSignature, UNiagaraComponent*, ReticleEffect);

/**
 * Ticking cosmetic task: spawns a reticle on the locally controlled client only and
 * moves it under the mouse cursor every frame. The reticle is either an actor
 * (ReticleClass) or a bare Niagara system (ReticleEffect) - if both are set, the actor
 * wins. No replication - the server and other clients never see it. The reticle is
 * destroyed automatically when the task's owning ability ends (release, interrupt or
 * cancel), so the ability graph needs no cleanup wiring.
 */
UCLASS()
class AURA_API UCursorReticle : public UAbilityTask
{
	GENERATED_BODY()

public:
	UCursorReticle();

	UFUNCTION(BlueprintCallable,
			Category = "Ability|Tasks",
			meta = (DisplayName = "CursorReticle", HidePin = "OwningAbility",
				DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UCursorReticle* CreateCursorReticle(UGameplayAbility* OwningAbility, TSubclassOf<AActor> ReticleClass,
											UNiagaraSystem* ReticleEffect = nullptr);

	// Fired right after the reticle spawns so the graph can keep a reference
	// (e.g. to scale it with charge). Only ever fires on the local client, and only
	// the one matching what was actually spawned (actor or Niagara component).
	UPROPERTY(BlueprintAssignable)
	FCursorReticleSpawnedSignature OnReticleSpawned;

	UPROPERTY(BlueprintAssignable)
	FCursorReticleEffectSpawnedSignature OnReticleEffectSpawned;

private:
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	bool GetCursorHit(FHitResult& OutHit) const;

	UPROPERTY()
	TObjectPtr<AActor> Reticle;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ReticleComponent;

	TSubclassOf<AActor> ReticleClass;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> ReticleEffect;
};
