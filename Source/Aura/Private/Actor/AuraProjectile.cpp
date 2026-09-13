// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "Aura/Aura.h"

// Sets default values
AAuraProjectile::AAuraProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	SetRootComponent(Sphere); 

	Sphere->SetGenerateOverlapEvents(true);
	Sphere->SetCollisionObjectType(ECC_Projectile);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	
}

void AAuraProjectile::EnableHoming(AActor* TargetActor)
{
	
	ProjectileMovement->HomingAccelerationMagnitude = 5.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->HomingTargetComponent =	TargetActor->GetRootComponent();
	ProjectileMovement->bIsHomingProjectile = true;
}

// Called when the game starts or when spawned
void AAuraProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(MaxLifeSpan);
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAuraProjectile::OnOverlap);

	LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, GetRootComponent());
}

void AAuraProjectile::OnHit()
{
	UGameplayStatics::PlaySoundAtLocation(this,
	                                      ImpactSound,
	                                      GetActorLocation(),
	                                      FRotator::ZeroRotator);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, GetActorLocation());
	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}
	bHit = true;
}

void AAuraProjectile::Destroyed()
{
	if (!bHit && !HasAuthority()) OnHit();

	Super::Destroyed();
}

void AAuraProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Instigator replicates, so this check works on clients too; DamageEffectParams
	// does NOT replicate — SourceASC is only safe to touch on the authority below
	AActor* SourceActor = GetInstigator();
	if (SourceActor == OtherActor) return;

	if (SourceActor && !UAuraAbilitySystemLibrary::IsNotFriend(SourceActor, OtherActor)) return;

	if (!bHit) OnHit();

	if (HasAuthority())
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			DamageEffectParams.TargetASC = TargetASC;
			FRotator Rotator = GetActorRotation();
			Rotator.Roll *= 1.1f;
			Rotator.Pitch *= 0.95f;
			Rotator.Yaw *= 0.95f;
			DamageEffectParams.DeathImpulse = Rotator.Vector() * DamageEffectParams.DeathImpulseMagnitude;
			
			FRotator Rotation = GetActorRotation();
			Rotation.Pitch = 45.f;
			
			DamageEffectParams.KnockbackImpulse = Rotation.Vector() * DamageEffectParams.KnockbackImpulseMagnitude;
			if (IsValid(DamageEffectParams.SourceASC))
			{
				UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
			}
		}
		Destroy();
	}
	else
	{
		bHit = true;
	}
}


