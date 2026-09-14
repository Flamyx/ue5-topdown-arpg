#include "AuraAbilityTypes.h"


bool FAuraGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint32 RepBits = 0;
	if (Ar.IsSaving())
	{
		if (bReplicateInstigator && Instigator.IsValid())
		{
			RepBits |= 1 << 0;
		}
		if (bReplicateEffectCauser && EffectCauser.IsValid())
		{
			RepBits |= 1 << 1;
		}
		if (AbilityCDO.IsValid())
		{
			RepBits |= 1 << 2;
		}
		if (bReplicateSourceObject && SourceObject.IsValid())
		{
			RepBits |= 1 << 3;
		}
		if (Actors.Num() > 0)
		{
			RepBits |= 1 << 4;
		}
		if (HitResult.IsValid())
		{
			RepBits |= 1 << 5;
		}
		if (bHasWorldOrigin)
		{
			RepBits |= 1 << 6;
		}
		if (bIsBlockedHit)
		{
			RepBits |= 1 << 7;
		}
		if (bIsCriticalHit)
		{
			RepBits |= 1 << 8;
		}
		// Bit 9 used to carry the scalar debuff fields too. Those are gone (debuffs are now
		// server-only arrays), but the damage type still replicates on this bit, so the
		// 13-bit layout below is unchanged.
		if (DamageType.IsValid())
		{
			RepBits |= 1 << 9;
		}
		if (!DeathImpulse.IsZero())
		{
			RepBits |= 1 << 10;
		}
		if (bIsDebuffHit)
		{
			RepBits |= 1 << 11;
		}
		if (!KnockbackImpulse.IsZero())
		{
			RepBits |= 1 << 12;
		}
	}

	// 13 bits are in use (0..12) — the count here must match or the archive
	// misaligns and every field after it is read as garbage on the remote side
	Ar.SerializeBits(&RepBits, 13);

	if (RepBits & (1 << 0))
	{
		Ar << Instigator;
	}
	if (RepBits & (1 << 1))
	{
		Ar << EffectCauser;
	}
	if (RepBits & (1 << 2))
	{
		Ar << AbilityCDO;
	}
	if (RepBits & (1 << 3))
	{
		Ar << SourceObject;
	}
	if (RepBits & (1 << 4))
	{
		SafeNetSerializeTArray_Default<31>(Ar, Actors);
	}
	if (RepBits & (1 << 5))
	{
		if (Ar.IsLoading())
		{
			if (!HitResult.IsValid())
			{
				HitResult = TSharedPtr<FHitResult>(new FHitResult());
			}
		}
		HitResult->NetSerialize(Ar, Map, bOutSuccess);
	}
	if (RepBits & (1 << 6))
	{
		Ar << WorldOrigin;
		bHasWorldOrigin = true;
	}
	else
	{
		bHasWorldOrigin = false;
	}
	if (RepBits & (1 << 7))
	{
		Ar << bIsBlockedHit;
	}
	if (RepBits & (1 << 8))
	{
		Ar << bIsCriticalHit;
	}
	if (RepBits & (1 << 9))
	{
		// Must serialize in BOTH directions — if only the reader consumes the tag
		// the bitstream misaligns and the remote side crashes/corrupts
		if (!DamageType.IsValid())
		{
			DamageType = TSharedPtr<FGameplayTag>(new FGameplayTag());
		}
		DamageType->NetSerialize(Ar, Map, bOutSuccess);
	}
	if  (RepBits & (1 << 10))
	{
		DeathImpulse.NetSerialize(Ar, Map, bOutSuccess);
	}
	if (RepBits & (1 << 11))
	{
		Ar << bIsDebuffHit;
	}
	if (RepBits & (1 << 12))
	{
		KnockbackImpulse.NetSerialize(Ar, Map, bOutSuccess);
	}

	if (Ar.IsLoading())
	{
		AddInstigator(Instigator.Get(), EffectCauser.Get()); // Just to initialize InstigatorAbilitySystemComponent
	}
	

	bOutSuccess = true;
	return true;
}
