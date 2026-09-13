// Copyright Sparrow Inc.


#include "AbilitySystem/GameplayAbility/MeteoriteProjectileSpell.h"
#include "AuraGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"


void UMeteoriteProjectileSpell::InputReleased(const FGameplayAbilitySpecHandle Handle, 
											const FGameplayAbilityActorInfo* ActorInfo,
											const FGameplayAbilityActivationInfo ActivationInfo)
{
	GetWorld()->GetTimerManager().ClearTimer(ChargeTimerHandle);
}

void UMeteoriteProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation)
{
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer) return;
	
	float lb = SpawnOffsetBounds[0], rb = SpawnOffsetBounds[1];
	const FVector2d SpawnOffset = { FMath::RandRange(-lb, lb), FMath::RandRange(-rb, rb) };
	const FVector ActorLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	const FVector SpawnLocation = ActorLocation + FVector(-SpawnOffset.X, -SpawnOffset.Y, SpawnHeight);
	FRotator Rotation = (ProjectileTargetLocation - SpawnLocation).Rotation();
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SpawnLocation);
	SpawnTransform.SetRotation(Rotation.Quaternion());

	AMeteoriteProjectile* Projectile = GetWorld()->SpawnActorDeferred<AMeteoriteProjectile>(
		ProjectileClass, SpawnTransform,
		GetOwningActorFromActorInfo(), Cast<APawn>(GetAvatarActorFromActorInfo()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile == nullptr) return;
	// ChargeTime comes either from the C++ ChargeTick timer (old graph) or straight
	// from WaitInputRelease's TimeHeld (new graph) — clamp so both paths are safe
	const float EffectiveCharge = FMath::Clamp(ChargeTime, 0.f, MaxChargeTime);
	Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Projectile->DamageEffectParams.Damage *= ChargedMultiplier.GetValueAtLevel(EffectiveCharge);
	Projectile->SetSphereRadius(MaxChargeTime > 0.f ? EffectiveCharge / MaxChargeTime : 1.f);

	Projectile->FinishSpawning(SpawnTransform);
}

void UMeteoriteProjectileSpell::StartChargeTimeline()
{
	GetWorld()->GetTimerManager().SetTimer(ChargeTimerHandle, this, &UMeteoriteProjectileSpell::ChargeTick, FChargeTick, true);
}

void UMeteoriteProjectileSpell::ChargeTick()
{
	ChargeTime = FMath::Clamp(ChargeTime + FChargeTick, 0.f, MaxChargeTime);
	CircleActorTick();
}
