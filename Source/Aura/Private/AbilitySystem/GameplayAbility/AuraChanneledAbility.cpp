// Copyright Sparrow Inc.


#include "AbilitySystem/GameplayAbility/AuraChanneledAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AbilityTasks/TargetDataUnderMouse.h"
#include "Character/AuraCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/CombatInterface.h"

void UAuraChanneledAbility::StartChannel()
{
	if (bChanneling) return;
	bChanneling = true;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return;

	if (ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		Character->GetCharacterMovement()->DisableMovement();
	}
	if (Avatar->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_SetInShockLoop(Avatar, true);
	}
	// Sample now instead of waiting one AimTickRate: on the owning client this fills
	// MouseHitLocation synchronously (and lets subclasses acquire a target immediately)
	RequestCursorTarget();

	if (AAuraCharacterBase* Caster = Cast<AAuraCharacterBase>(Avatar))
	{
		// Zero means no sample yet - the server before the client's first RPC lands, or an ability
		// whose Blueprint never set MouseHitLocation (Meteorite). Aim straight ahead then, so the
		// caster doesn't swing toward world origin while waiting.
		const FVector InitialAim = MouseHitLocation.IsNearlyZero()
			? Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.f
			: MouseHitLocation;
		Caster->BeginChanneling(InitialAim);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AimTimerHandle, this, &UAuraChanneledAbility::AimTick, AimTickRate, true);
	}
}

void UAuraChanneledAbility::EndChannel()
{
	// No early-out on !bChanneling: Blueprints may have locked movement or set the pose
	// themselves, and every call below is idempotent
	bChanneling = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AimTimerHandle);
	}
	if (TargetDataTask)
	{
		TargetDataTask->EndTask();
		TargetDataTask = nullptr;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return;

	if (Avatar->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_SetInShockLoop(Avatar, false);
	}
	if (ACharacter* Character = Cast<ACharacter>(Avatar))
	{
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	if (AAuraCharacterBase* Caster = Cast<AAuraCharacterBase>(Avatar))
	{
		Caster->EndChanneling();
	}
}

void UAuraChanneledAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	EndChannel();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraChanneledAbility::SetCasterAimLocation(const FVector& Location) const
{
	if (AAuraCharacterBase* Caster = Cast<AAuraCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Caster->SetAimLocation(Location);
	}
}

void UAuraChanneledAbility::AimTick()
{
	RequestCursorTarget();
	// On the owning client the callback above ran synchronously, so MouseHitLocation is this
	// frame's cursor (already refined by OnAimUpdated). On the server it is the newest sample
	// the client has sent so far - at most one tick stale, or still zero if none has arrived,
	// in which case keep the seeded aim rather than turning toward world origin.
	if (!MouseHitLocation.IsNearlyZero())
	{
		SetCasterAimLocation(MouseHitLocation);
	}
}

void UAuraChanneledAbility::RequestCursorTarget()
{
	if (!IsActive()) return;

	// Tasks end after one sample; this only drops a server-side one still waiting when the next
	// tick comes round, so waiting tasks don't pile up on a slow connection
	if (TargetDataTask)
	{
		TargetDataTask->EndTask();
		TargetDataTask = nullptr;
	}

	TargetDataTask = UTargetDataUnderMouse::CreateTargetDataUnderMouse(this);
	TargetDataTask->ValidData.AddDynamic(this, &UAuraChanneledAbility::OnTargetDataReceived);
	TargetDataTask->ReadyForActivation();   // without this, Activate() never runs
}

void UAuraChanneledAbility::OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data)
{
	const FHitResult Hit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Data, 0);
	if (!Hit.bBlockingHit) return;   // the task always sends, hit or miss

	MouseHitLocation = Hit.ImpactPoint;
	MouseHitActor = Hit.GetActor();
	OnAimUpdated(Hit);
}
