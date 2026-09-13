// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayAbility/AuraProjectileSpell.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "Interaction/CombatInterface.h"

void UAuraProjectileSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (false)
		UKismetSystemLibrary::PrintString(this, FString("Activate Ability cpp"), true, true, FLinearColor::Yellow, 3.f);

}

void UAuraProjectileSpell::SpawnProjectile(const FVector& ProjectileTargetLocation)
{
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer) return;

	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(GetAvatarActorFromActorInfo(), 
																					FAuraGameplayTags::Get().CombatSocket_Weapon);
	FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
	//Rotation.Pitch = 0.f;

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SocketLocation);
	SpawnTransform.SetRotation(Rotation.Quaternion());

	// Instigator must be the avatar pawn — it replicates, and clients use it for
	// the friend/self checks in OnOverlap (the owning actor is the PlayerState)
	AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(ProjectileClass, SpawnTransform,
		GetOwningActorFromActorInfo(), Cast<APawn>(GetAvatarActorFromActorInfo()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Projectile->FinishSpawning(SpawnTransform);
	
	// const UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());
	//
	// FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
	// EffectContextHandle.SetAbility(this);
	// EffectContextHandle.AddSourceObject(Projectile);
	// //WeakObjectPtr because Handle references are not always accounted in Garbage Collection
	// TArray<TWeakObjectPtr<AActor>> Actors; 
	// Actors.Add(Projectile);
	// EffectContextHandle.AddActors(Actors);
	// FHitResult HitResult;
	// HitResult.Location = ProjectileTargetLocation;
	// EffectContextHandle.AddHitResult(HitResult);
	//
	// const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), EffectContextHandle);
	//
	// UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DamageType, Damage.GetValueAtLevel(GetAbilityLevel()));
	//
	// if (Projectile == nullptr)
	// 	return;
	// Projectile->DamageEffectSpecHandle = SpecHandle;
}
