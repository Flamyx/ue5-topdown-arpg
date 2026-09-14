#pragma once

#include "GameplayEffectTypes.h"
#include "AuraAbilityTypes.generated.h"

class UGameplayEffect;

/**
 * One debuff an ability can apply on hit. Abilities carry a list of these, so a single
 * spell can apply several (Meteorite = Burn + Stun) and two spells of the same damage
 * type can differ (FireBolt burns, Meteorite also stuns). Each entry is rolled on its own
 * against the target's resistance to the hit's damage type.
 */
USTRUCT(BlueprintType)
struct FAuraDebuffSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (Categories = "Debuff"))
	FGameplayTag DebuffTag;

	/** Percent, 0..100, before resistance. 100 always lands against zero resistance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
	float Chance = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Duration = 0.f;

	/** Seconds between damage ticks while the debuff is active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Frequency = 1.f;

	/** Damage per tick. 0 for pure control debuffs like Stun. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0"))
	float Damage = 0.f;
};

USTRUCT(BlueprintType)
struct FAuraDamageEffectParams
{
	GENERATED_BODY()
	
	FAuraDamageEffectParams(){};
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<class UObject> WorldContextObject = nullptr;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr;
	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> ResidualDamageEffectClass = nullptr;
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> TargetASC = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag DamageType = FGameplayTag();
	
	UPROPERTY(BlueprintReadWrite)
	float Damage = 0.f;
	UPROPERTY(BlueprintReadWrite)
	float AbilityLevel = 0.f;
	UPROPERTY(BlueprintReadWrite)
	TArray<FAuraDebuffSpec> Debuffs;

	
	UPROPERTY(BlueprintReadWrite)
	float DeathImpulseMagnitude = 0.f;
	UPROPERTY(BlueprintReadWrite)
	FVector DeathImpulse = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadWrite)
	float KnockbackImpulseMagnitude = 0.f;
	UPROPERTY(BlueprintReadWrite)
	FVector KnockbackImpulse = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FAuraGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:

	bool IsCriticalHit() const { return bIsCriticalHit; }
	bool IsBlockedHit() const { return bIsBlockedHit; }
	bool IsDebuffHit() const { return bIsDebuffHit; }
	bool IsSuccessfulDebuff() const { return SuccessfulDebuffs.Num() > 0; }
	const TArray<FAuraDebuffSpec>& GetPendingDebuffs() const { return PendingDebuffs; }
	const TArray<FAuraDebuffSpec>& GetSuccessfulDebuffs() const { return SuccessfulDebuffs; }
	FGameplayTag GetDamageType() const { return DamageType.IsValid() ? *DamageType : FGameplayTag(); }
	FVector GetDeathImpulse() const { return DeathImpulse; }
	FVector GetKnockbackImpulse() const { return KnockbackImpulse; }

	void SetIsCriticalHit(bool Value) { bIsCriticalHit = Value;  }
	void SetIsBlockedHit(bool Value) { bIsBlockedHit = Value;  }
	void SetIsDebuffHit(bool Value) { bIsDebuffHit = Value;  }
	void SetPendingDebuffs(const TArray<FAuraDebuffSpec>& Value) { PendingDebuffs = Value; }
	void AddSuccessfulDebuff(const FAuraDebuffSpec& Value) { SuccessfulDebuffs.Add(Value); }
	void SetDamageType(TSharedPtr<FGameplayTag> Value) { DamageType = Value; }
	void SetDeathImpulse(const FVector& Value) { DeathImpulse = Value; }
	void SetKnockbackImpulse(const FVector& Value) { KnockbackImpulse = Value; }

	/**
	 * Checked downcast from a handle. The engine builds without RTTI, so dynamic_cast is unavailable;
	 * reflection tells us what was actually allocated instead. Null when the handle is empty or holds
	 * a plain FGameplayEffectContext - AbilitySystemGlobalsClassName not pointing at
	 * UAuraAbilitySystemGlobals in the running build's config is how that happens.
	 */
	static FAuraGameplayEffectContext* ExtractEffectContext(FGameplayEffectContextHandle& Handle)
	{
		return CheckedDowncast(Handle.Get());
	}
	static const FAuraGameplayEffectContext* ExtractEffectContext(const FGameplayEffectContextHandle& Handle)
	{
		return CheckedDowncast(const_cast<FGameplayEffectContext*>(Handle.Get()));
	}

	/** Returns the actual struct used for serialization, subclasses must override this! */
	virtual UScriptStruct* GetScriptStruct() const
	{
		return StaticStruct();
	}

	/** Creates a copy of this context, used to duplicate for later modifications */
	virtual FAuraGameplayEffectContext* Duplicate() const
	{
		FAuraGameplayEffectContext* NewContext = new FAuraGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

protected:

	UPROPERTY()
	bool bIsBlockedHit = false;

	UPROPERTY()
	bool bIsCriticalHit = false;
	
	UPROPERTY()
	bool bIsDebuffHit = false;
	
	// Server-only, and consumed in the same frame: ApplyDamageEffect fills PendingDebuffs,
	// UExecCalc_Damage rolls them into SuccessfulDebuffs, UAuraAttributeSet applies those.
	// Deliberately not NetSerialized - nothing on a client reads them.
	UPROPERTY()
	TArray<FAuraDebuffSpec> PendingDebuffs;

	UPROPERTY()
	TArray<FAuraDebuffSpec> SuccessfulDebuffs;

	UPROPERTY()
	FVector DeathImpulse = FVector::ZeroVector;
	
	UPROPERTY()
	FVector KnockbackImpulse = FVector::ZeroVector;
	
	TSharedPtr<FGameplayTag> DamageType;

private:
	static FAuraGameplayEffectContext* CheckedDowncast(FGameplayEffectContext* Base)
	{
		if (Base == nullptr) return nullptr;
		if (!ensureMsgf(Base->GetScriptStruct()->IsChildOf(StaticStruct()),
			TEXT("Effect context is %s, not FAuraGameplayEffectContext - check AbilitySystemGlobalsClassName in DefaultGame.ini"),
			*Base->GetScriptStruct()->GetName()))
		{
			return nullptr;
		}
		return static_cast<FAuraGameplayEffectContext*>(Base);
	}
};

template<>
struct TStructOpsTypeTraits< FAuraGameplayEffectContext > : public TStructOpsTypeTraitsBase2< FAuraGameplayEffectContext >
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		// Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};