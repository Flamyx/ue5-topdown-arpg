// Copyright Sparrow Inc.


#include "Actor/MeteoriteProjectile.h"
#include "Aura/Aura.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"


AMeteoriteProjectile::AMeteoriteProjectile()
{
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	
	ProjectileMovement->InitialSpeed = 400.f;
	ProjectileMovement->MaxSpeed = 500.f;
	ProjectileMovement->bRotationFollowsVelocity = 1;
	// Zero gravity: the spell aims the meteor straight at the target location, and any
	// gravity pulls it below that line so it lands short (400 u/s = long flight time)
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AMeteoriteProjectile::SetSphereRadius(float ChargeRatio)
{
	BlastRadius = FMath::Lerp(BlastRadius, MaxBlastRadius, ChargeRatio);
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::SanitizeFloat(BlastRadius, 2));
}

void AMeteoriteProjectile::BeginPlay()
{
	// Super already sets the life span, spawns the looping sound and binds the overlap.
	// Do NOT rebind here: dynamic delegates bind by function *name*, so the parent's
	// AddDynamic(this, &AAuraProjectile::OnOverlap) already dispatches to this class's
	// override - binding "OnOverlap" a second time is a duplicate and trips the
	// InvocationList[CurFunctionIndex] != InDelegate ensure in ScriptDelegates.h.
	Super::BeginPlay();
}

void AMeteoriteProjectile::Destroyed()
{
	if (!bHit && !HasAuthority())
	{
		OnHit();
	}
	Super::Destroyed();
}

void AMeteoriteProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Instigator replicates; DamageEffectParams.SourceASC is null on clients
	AActor* Avatar = GetInstigator();

	if (Avatar == OtherActor || (Avatar && !UAuraAbilitySystemLibrary::IsNotFriend(OtherActor, Avatar)))
		return;

	if (IsPendingKillPending())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("PendingKill"));
		return;
	}

	const bool bFirstHit = !bHit;
	if (bFirstHit)
		OnHit();

	if (HasAuthority() && bFirstHit && IsValid(DamageEffectParams.SourceASC))
	{
		const TArray<AActor*> IgnoredActors = { Avatar };
		TArray<AActor*> AffectedActors;
		
		UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(Avatar, AffectedActors, IgnoredActors, BlastRadius, GetActorLocation());

		for (AActor* AffectedActor : AffectedActors)
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AffectedActor))
			{
				DamageEffectParams.TargetASC = TargetASC;
				DamageEffectParams.DeathImpulse = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;
				UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
			}
		}
		
		Destroy();
	}

	else bHit = true;
}