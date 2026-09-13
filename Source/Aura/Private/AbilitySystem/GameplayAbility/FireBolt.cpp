// Copyright Sparrow Inc.


#include "AbilitySystem/GameplayAbility/FireBolt.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"

void UFireBolt::SpawnProjectiles(const FVector& ProjectileTargetLocation, AActor* HomingTarget, bool bOverridePitch, float PitchOverride)
{
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer) return;
	
	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(GetAvatarActorFromActorInfo(), FAuraGameplayTags::Get().CombatSocket_Weapon);
	const FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();

	const int32 NumProjectiles = FMath::Min(ProjectileLimit, GetAbilityLevel());
	TArray<FRotator> Rotations = UAuraAbilitySystemLibrary::GetEvenlyScacedRotators(Rotation.Vector(), FVector::UpVector, Spread, NumProjectiles);
	for (const FRotator& Rot : Rotations)
	{
		FTransform Transform = FTransform();
		Transform.SetRotation(Rot.Quaternion());
		Transform.SetLocation(SocketLocation);

		auto Bolt = GetWorld()->SpawnActorDeferred<AAuraProjectile>(
			ProjectileClass, Transform,
			GetOwningActorFromActorInfo(), Cast<APawn>(GetAvatarActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Bolt == nullptr) continue;
		Bolt->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();

		if (IsValid(HomingTarget) && HomingTarget->Implements<UCombatInterface>())
		{
			Bolt->ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();
		}
		//static meshes case
		else
		{
			// Outer must be the projectile so the transient component isn't GC'd mid-flight
			Bolt->HomingTargetComponent = NewObject<USceneComponent>(Bolt);
			Bolt->HomingTargetComponent->SetWorldLocation(ProjectileTargetLocation);
			Bolt->ProjectileMovement->HomingTargetComponent = Bolt->HomingTargetComponent;
		}
		Bolt->ProjectileMovement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
		Bolt->ProjectileMovement->bIsHomingProjectile = bLaunchHomingProjectiles;
		Bolt->FinishSpawning(Transform);
		ProjectileActors.Add(Bolt);
	}
}

void UFireBolt::HomeToTarget(AAuraProjectile* Projectile, AActor* HomingTarget, bool bOverridePitch, float PitchOverride)
{
	Projectile->EnableHoming(HomingTarget);
	
}
