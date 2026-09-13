// Copyright Sparrow Inc.


#include "AbilitySystem/AbilityTasks/CursorReticle.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

UCursorReticle::UCursorReticle()
{
	bTickingTask = true;
}

UCursorReticle* UCursorReticle::CreateCursorReticle(UGameplayAbility* OwningAbility, TSubclassOf<AActor> ReticleClass,
													UNiagaraSystem* ReticleEffect)
{
	UCursorReticle* Task = NewAbilityTask<UCursorReticle>(OwningAbility);
	Task->ReticleClass = ReticleClass;
	Task->ReticleEffect = ReticleEffect;
	return Task;
}

void UCursorReticle::Activate()
{
	// Cosmetic only: the server and simulated proxies have no cursor and get no reticle
	if (!Ability->GetCurrentActorInfo()->IsLocallyControlled()) return;
	if (ReticleClass == nullptr && ReticleEffect == nullptr) return;

	// Spawn at the cursor if it hits something, so the reticle doesn't flash at the
	// avatar's feet for one frame
	FHitResult CursorHit;
	const FVector SpawnLocation = GetCursorHit(CursorHit)
		? CursorHit.ImpactPoint
		: GetAvatarActor()->GetActorLocation();

	if (ReticleClass != nullptr)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Reticle = GetWorld()->SpawnActor<AActor>(ReticleClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		if (Reticle != nullptr)
		{
			OnReticleSpawned.Broadcast(Reticle);
		}
	}
	else
	{
		// bAutoDestroy must stay false: the task owns the component's lifetime, and for
		// a looping system auto-destroy would never trigger anyway
		ReticleComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ReticleEffect, SpawnLocation, FRotator::ZeroRotator, FVector(1.f),
			/*bAutoDestroy=*/false);
		if (ReticleComponent != nullptr)
		{
			OnReticleEffectSpawned.Broadcast(ReticleComponent);
		}
	}
}

void UCursorReticle::TickTask(float DeltaTime)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan,
			FString::Printf(TEXT("CursorReticle::TickTask DeltaTime=%f"), DeltaTime));
	}
	Super::TickTask(DeltaTime);
	if (!IsValid(Reticle) && !IsValid(ReticleComponent)) return;

	FHitResult CursorHit;
	if (!GetCursorHit(CursorHit)) return;

	if (IsValid(Reticle))
	{
		Reticle->SetActorLocation(CursorHit.ImpactPoint);
	}
	else
	{
		ReticleComponent->SetWorldLocation(CursorHit.ImpactPoint);
	}
	
}

void UCursorReticle::OnDestroy(bool bInOwnerFinished)
{
	if (IsValid(Reticle))
	{
		Reticle->Destroy();
	}
	if (IsValid(ReticleComponent))
	{
		ReticleComponent->DestroyComponent();
	}
	Super::OnDestroy(bInOwnerFinished);
}

bool UCursorReticle::GetCursorHit(FHitResult& OutHit) const
{
	const APlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.Get();
	if (PC == nullptr) return false;
	return PC->GetHitResultUnderCursor(ECC_Visibility, false, OutHit) && OutHit.bBlockingHit;
}
