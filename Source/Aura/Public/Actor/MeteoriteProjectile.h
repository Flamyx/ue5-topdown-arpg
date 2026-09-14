// Copyright Sparrow Inc.

#pragma once

#include "CoreMinimal.h"
#include "AuraProjectile.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "MeteoriteProjectile.generated.h"

class UNiagaraSystem;

/**
 * 
 */
UCLASS()
class AURA_API AMeteoriteProjectile : public AAuraProjectile
{
	GENERATED_BODY()

public:
	AMeteoriteProjectile();

	/** 0..1 charge fraction, set by the spell before FinishSpawning. Replicated so every machine
	 *  derives the same blast radius and visual size in BeginPlay; BlastRadius itself doesn't replicate. */
	UPROPERTY(Replicated)
	float ChargeRatio = 0.f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual FVector GetImpactEffectScale() const override { return FVector(GetChargeScale()); }

	virtual void OnOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult) override;

	/** Blast radius at zero charge; lerps toward MaxBlastRadius with ChargeRatio in BeginPlay. */
	UPROPERTY(EditAnywhere)
	float BlastRadius;
	UPROPERTY(EditAnywhere)
	float MaxBlastRadius;

private:
	/** Visual scale = gameplay radius / uncharged radius, so the meteor and its explosion match the damage area */
	float GetChargeScale() const { return BaseBlastRadius > 0.f ? BlastRadius / BaseBlastRadius : 1.f; }

	float BaseBlastRadius = 0.f;
};
