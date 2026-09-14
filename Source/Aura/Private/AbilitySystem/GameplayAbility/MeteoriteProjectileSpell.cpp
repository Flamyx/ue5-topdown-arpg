// Copyright Sparrow Inc.


#include "AbilitySystem/GameplayAbility/MeteoriteProjectileSpell.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

float UMeteoriteProjectileSpell::GetManaCostMultiplier() const
{
	return ChargedManaCostMultiplier.GetValueAtLevel(GetEffectiveCharge());
}

void UMeteoriteProjectileSpell::CommitExecute(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Only reached when CommitAbility's cost and cooldown checks passed
	Super::CommitExecute(Handle, ActorInfo, ActivationInfo);
	bCommitted = true;
}

void UMeteoriteProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation)
{
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer) return;
	if (!bCommitted) return;   // couldn't pay for this charge: the meteor fizzles

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

	// ChargeTime comes straight from WaitInputRelease's TimeHeld; clamp to the charge window
	const float EffectiveCharge = GetEffectiveCharge();
	Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Projectile->DamageEffectParams.Damage *= ChargedMultiplier.GetValueAtLevel(EffectiveCharge);
	// Replicated, and set before FinishSpawning so it arrives with the actor's initial state:
	// every machine derives the same blast radius and visual scale from it in BeginPlay
	Projectile->ChargeRatio = MaxChargeTime > 0.f ? EffectiveCharge / MaxChargeTime : 1.f;

	Projectile->FinishSpawning(SpawnTransform);
}
