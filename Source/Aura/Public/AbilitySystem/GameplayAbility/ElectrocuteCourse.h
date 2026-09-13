// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbility/AuraDamageGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "ElectrocuteCourse.generated.h"

class UTargetDataUnderMouse;
/**
 *
 */
UCLASS()
class AURA_API UElectrocuteCourse : public UAuraDamageGameplayAbility
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

	UFUNCTION(BlueprintCallable)
	void AbilityTick();

	UFUNCTION()   // required - ValidData is a DYNAMIC delegate
	void OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data);

	UFUNCTION()
	void RequestCursorTarget();

	UPROPERTY()
	TObjectPtr<UTargetDataUnderMouse> TargetDataTask;

protected:
	UPROPERTY(BlueprintReadWrite)
	FVector MouseHitLocation;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> MouseHitActor;

	UPROPERTY(EditDefaultsOnly)
	float ForkRadius;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<AActor>> AffectedActors;

	UFUNCTION(BlueprintCallable)
	void AffectActors();

	UFUNCTION(BlueprintCallable)
	void RemoveEffects();

	UFUNCTION(BlueprintCallable)
	void DamageActors();

	UFUNCTION(BlueprintCallable)
	void RunDamageLogic();

	UPROPERTY(EditDefaultsOnly)
	float FChargeTick;
	
	UPROPERTY(EditDefaultsOnly)
	float TargetGraceSeconds;
	
	double LastValidTargetTime = 0.f;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;


private:
	FTimerHandle ChargeTimerHandle;

	void DamageTarget(AActor* TargetActor);
};
