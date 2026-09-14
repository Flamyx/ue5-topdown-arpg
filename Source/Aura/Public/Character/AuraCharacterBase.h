// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "Interaction/CombatInterface.h"
#include "AuraCharacterBase.generated.h"

UCLASS()
class AURA_API AAuraCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAuraCharacterBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const { return AttributeSet; } 

	/* Combat Interface */
	virtual void Die(const FVector& DeathImpulse) override;
	virtual void Knockback(const FVector& KnockbackImpulse) override;
	virtual UAnimMontage* GetHitReactMontage_Implementation() override;
	virtual FVector GetCombatSocketLocation_Implementation(const FGameplayTag& SocketTag) override;
	virtual bool IsDead_Implementation() const override;
	virtual AActor* GetAvatar_Implementation() override;
	virtual UNiagaraSystem* GetBloodEffect_Implementation() override;
	virtual int32 GetMinionCount_Implementation() override { return MinionCount; };
	virtual int32 GetMinionLimit_Implementation() override { return MinionLimit; };
	virtual void UpdateMinionCount_Implementation(int32 AddVal) override { MinionCount += AddVal; };
	virtual ECharacterClass GetCharacterClass_Implementation() override;
	virtual FOnASCRegistered& GetOnASCRegisteredDelegate() override;
	virtual FOnDeath& GetOnDeathDelegate() override;
	virtual USkeletalMeshComponent* GetWeapon_Implementation() override;
	/* End Combat Interface */

	UFUNCTION(NetMulticast, Reliable)
	virtual void MulticastHandleDeath(const FVector& DeathImpulse);
 
	bool isNPC = false;
	
	FOnASCRegistered OnASCRegisteredDelegate;
	FOnDeath OnDeathDelegate;
	
	/*
	Channeling (driven by UAuraChanneledAbility on the server and the owning client)
	*/

	void BeginChanneling(const FVector& InitialAim);
	void EndChanneling();
	void SetAimLocation(const FVector& Location) { AimLocation = Location; }

	/** Per-frame smoothed aim point: the beam end GC_ShockLoop draws to, and what the caster turns
	 *  to face while channeling. Kept under this name so existing Blueprint calls still resolve. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	FVector GetBeamEndLocation() const { return SmoothedAimLocation; }

	/** True on EVERY machine while channeling: set locally by the server and the owning client,
	 *  replicated to everyone else. Drive the channel pose from this in the AnimBP - the old
	 *  SetInShockLoop event only fires where the ability runs, so other clients never saw the loop. */
	UFUNCTION(BlueprintPure, Category = "Combat|Channel")
	bool IsChanneling() const { return bIsChanneling; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FName WeaponTipSocketName;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TMap<FGameplayTag, FName> TagToWeaponSocketName;

	bool bDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Class Defaults")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultSecondaryAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;

	virtual void InitAbilityActorInfo();

	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffect, float Level) const;
	virtual void InitializeDefaultAttributes() const;

	void AddCharacterAbilities();

	/*
	Dissolve Effects
	*/

	void Dissolve();

	UFUNCTION(BlueprintImplementableEvent)
	void StartDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);

	UFUNCTION(BlueprintImplementableEvent)
	void StartWeaponDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInstance> WeaponDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UNiagaraSystem* BloodEffect;

	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> DeathSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MinionCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MinionLimit = 0;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDebuffNiagaraComponent> BurnDebuffComponent;
	
	UFUNCTION(Server, Reliable)
	void HandleKnockback(const FVector& KnockbackImpulse);

	/** Raw aim sample, written every aim tick. SkipOwner: the owner writes its own locally fresh
	 *  cursor value, and the server's older copy would stomp it and make the beam end jitter. */
	UPROPERTY(Replicated)
	FVector_NetQuantize AimLocation;

	UPROPERTY(ReplicatedUsing = OnRep_IsChanneling)
	bool bIsChanneling = false;

	UFUNCTION()
	void OnRep_IsChanneling();

	FVector SmoothedAimLocation = FVector::ZeroVector;

	/** How quickly the smoothed aim point chases the raw samples. Higher = snappier sweep. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Channel", meta = (ClampMin = "0"))
	float AimInterpSpeed = 15.f;

	/** How quickly the caster turns toward the aim point while channeling. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Channel", meta = (ClampMin = "0"))
	float ChannelTurnSpeed = 10.f;

private:
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupPassiveAbilities;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;
};
