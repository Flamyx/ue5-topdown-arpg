// Copyright Sparrow Inc.


#include "AbilitySystem/GameplayAbility/AuraDamageGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"

UAuraDamageGameplayAbility::UAuraDamageGameplayAbility()
{
	bReplicateInputDirectly = true;
}

void UAuraDamageGameplayAbility::CauseDamage(AActor* TargetActor)
{
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (Spec == nullptr || TargetASC == nullptr) return;

	Spec->SetSetByCallerMagnitude(DamageType, Damage.GetValueAtLevel(GetAbilityLevel()));
	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Spec, TargetASC);
}

FAuraDamageEffectParams UAuraDamageGameplayAbility::MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor) const
{
	FAuraDamageEffectParams DamageEffectParams;
	DamageEffectParams.WorldContextObject = GetAvatarActorFromActorInfo();
	DamageEffectParams.AbilityLevel = GetAbilityLevel();
	
	DamageEffectParams.DamageEffectClass = DamageEffectClass;
	
	DamageEffectParams.SourceASC = GetAbilitySystemComponentFromActorInfo();
	DamageEffectParams.TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	
	DamageEffectParams.Damage = Damage.GetValueAtLevel(GetAbilityLevel());
	DamageEffectParams.DamageType = DamageType;
	
	DamageEffectParams.DebuffChance = DebuffChance;
	DamageEffectParams.DebuffDamage = DebuffDamage;
	DamageEffectParams.DebuffFrequency = DebuffFrequency;
	DamageEffectParams.DebuffDuration = DebuffDuration;
	
	DamageEffectParams.DeathImpulseMagnitude = DeathImpulseMagnitude;
	DamageEffectParams.KnockbackImpulseMagnitude = KnockbackImpulseMagnitude;
	
	if (IsValid(TargetActor))
	{
		FRotator Direction = (TargetActor->GetActorLocation() - GetAvatarActorFromActorInfo()->GetActorLocation()).Rotation();
		Direction.Pitch = 45.f;
		DamageEffectParams.KnockbackImpulse = Direction.Vector() * KnockbackImpulseMagnitude;
		DamageEffectParams.DeathImpulse = Direction.Vector() * DeathImpulseMagnitude;
	}
	
	return DamageEffectParams;
}

void UAuraDamageGameplayAbility::StoreOwnerVariables()
{
	OwnerPlayerController = GetCurrentActorInfo()->PlayerController.Get();
	OwnerCharacter = Cast<ACharacter>(CurrentActorInfo->AvatarActor);
}
