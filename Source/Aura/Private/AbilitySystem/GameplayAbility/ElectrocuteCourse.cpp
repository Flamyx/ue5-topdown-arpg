// Copyright Sparrow Inc.


#include "AbilitySystem/GameplayAbility/ElectrocuteCourse.h"

#include "AuraGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AbilitySystem/AbilityTasks/TargetDataUnderMouse.h"
#include "Character/AuraCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/EnemyInterface.h"

namespace
{
	// Cosmetic, and only for the player doing the casting. The ability also runs on the server,
	// so on a listen server the server-side instance of a REMOTE client's cast would otherwise
	// light that client's target up on the host's screen.
	void SetEnemyHighlighted(const UGameplayAbility* Ability, AActor* Actor, bool bHighlight)
	{
		const FGameplayAbilityActorInfo* Info = Ability->GetCurrentActorInfo();
		if (!Info || !Info->IsLocallyControlled()) return;

		if (IEnemyInterface* Enemy = Cast<IEnemyInterface>(Actor))
		{
			if (bHighlight) Enemy->HighlightActor();
			else            Enemy->UnHighlightActor();
		}
	}
}


void UElectrocuteCourse::TraceFirstTarget(const FVector& TargetLocation)
{
	if (!ensureAlways(IsValid(OwnerCharacter))) return;
	if (OwnerCharacter->Implements<UCombatInterface>())
	{
		const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(OwnerCharacter, FAuraGameplayTags::Get().CombatSocket_Weapon);
		const TArray<AActor*> ActorsToIgnore = { OwnerCharacter };
		FHitResult HitResult;
		UKismetSystemLibrary::SphereTraceSingle(
			OwnerCharacter,
			SocketLocation,
			TargetLocation,
			10.f,
			ETraceTypeQuery::TraceTypeQuery1,
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			HitResult,
			true
		);
		
		if (HitResult.bBlockingHit)
		{
			MouseHitLocation = HitResult.ImpactPoint;
			MouseHitActor = HitResult.GetActor();
		}

		// IsNotFriend is what stops Aura targeting HERSELF: ECC_Target blocks by default and
		// nothing sets a response for it on her capsule/mesh, so a cursor near her feet
		// hits her own body. The sphere trace then ignores her and misses, MouseHitActor
		// stays Aura, and she implements CombatInterface. Both carry the "Player" tag, so
		// IsNotFriend also rules out allies. Forks are already filtered this way inside
		// GetLivePlayersWithinRadius - the primary never was.
		const double Now = GetWorld()->GetTimeSeconds();
		if (IsValid(MouseHitActor)
			&& MouseHitActor->Implements<UCombatInterface>()
			&& UAuraAbilitySystemLibrary::IsNotFriend(OwnerCharacter, MouseHitActor))
		{
			LastValidTargetTime = Now;
			SetPrimaryTarget(MouseHitActor);                        // acquire instantly
		}
		else if (Now - LastValidTargetTime > TargetGraceSeconds)
		{
			SetPrimaryTarget(nullptr);                              // release only after the window
		}
	}
}

void UElectrocuteCourse::StoreHitResult(const FHitResult& HitResult)
{
	if (HitResult.bBlockingHit)
	{
		MouseHitActor = HitResult.GetActor();
		MouseHitLocation = HitResult.ImpactPoint;
	}
	else
	{
		CancelAbility(GetCurrentAbilitySpecHandle(), CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UElectrocuteCourse::UnbindPrimary()
{
	// The beam cue is deliberately NOT touched here - it lives on the caster for the whole
	// channel (see RunDamageLogic), so a retarget just moves its endpoint instead of
	// destroying and respawning the Niagara system and its looping sound.
	if (IsValid(BoundPrimary))
	{
		if (BoundPrimary->Implements<UCombatInterface>())
		{
			Cast<ICombatInterface>(BoundPrimary)->GetOnDeathDelegate()
				.RemoveDynamic(this, &UElectrocuteCourse::PrimaryActorDied);
		}
		// The primary highlight lives here rather than in the player controller: the controller
		// only knows the cast-time hover target, this follows every retarget, and every exit
		// path (grace release, death, EndAbility) already funnels through UnbindPrimary.
		SetEnemyHighlighted(this, BoundPrimary, false);
	}
	RemoveEffects();   // fork cues are attached to the primary, so they go with it
	BoundPrimary = nullptr;
}

void UElectrocuteCourse::SetPrimaryTarget(AActor* NewTarget)
{
	if (BoundPrimary == NewTarget) return;   // same target - no rebind, no ensure

	UnbindPrimary();

	if (!IsValid(NewTarget) || !NewTarget->Implements<UCombatInterface>()) return;
	BoundPrimary = NewTarget;
	
	Cast<ICombatInterface>(BoundPrimary)->GetOnDeathDelegate()
			.AddDynamic(this, &UElectrocuteCourse::PrimaryActorDied);
	SetEnemyHighlighted(this, BoundPrimary, true);
}

void UElectrocuteCourse::AffectActors()
{
	if (GetAbilityLevel() < 2) return;
	// BoundPrimary, not MouseHitActor: the latter is the raw trace result and is perfectly
	// valid when it IS the floor, which would fork off a cursor pointed at bare ground.
	if (!IsValid(BoundPrimary)) return;

	// Authority only. This ability is LocalPredicted, so the predicting client runs this
	// too - but AddGameplayCue early-outs unless the owner actor is authoritative (the
	// target's ASC holds no prediction key of ours), and the server's cue replicates to
	// every client by itself. On a client this only burns a sphere overlap and fills
	// AffectedActors with entries whose cues it can never add or remove.
	if (!HasAuthority(&CurrentActivationInfo)) return;

	TArray<AActor*> Actors = TArray<AActor*>();
	const TArray<AActor*> ActorsToIgnore = { GetAvatarActorFromActorInfo(), BoundPrimary };
	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		GetAvatarActorFromActorInfo(), 
		Actors,
		ActorsToIgnore,
		ForkRadius,
		MouseHitLocation);
	
	// Drop the cues of actors that left the fork set, THEN drop the entries. Cue first:
	// RemoveAll would discard the only handle we have to remove the cue with, and an
	// orphaned looping cue keeps a beam and sfx_ShockLoop running on that actor forever.
	for (AActor* Affected : AffectedActors)
	{
		if (IsValid(Affected) && Actors.Contains(Affected)) continue;
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Affected))
		{
			ASC->RemoveGameplayCue(FAuraGameplayTags::Get().GameplayCue_ShockLoop_Fork);
		}
	}
	AffectedActors.RemoveAll([&Actors](const AActor* Affected)
	{
		return !IsValid(Affected) || !Actors.Contains(Affected);
	});
	//Remove AlreadyAffected
	Actors.RemoveAll( [this] (AActor* Actor)
	{
		return !IsValid(Actor) || AffectedActors.Contains(Actor);
	});
	
	// Add new candidates into whatever slots are free. NumToAdd is computed ONCE: as a
	// loop condition it would shrink on every Add, so only half the forks get attached.
	const int32 NumToAdd = FMath::Min(GetAbilityLevel() - AffectedActors.Num(), Actors.Num());
	for (int32 i = 0; i < NumToAdd; ++i)
	{
		
		auto TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actors[i]);
		if (!IsValid(TargetASC)) continue;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Actor: " + Actors[i]->GetName());
		FGameplayCueParameters CueParams = FGameplayCueParameters();
		CueParams.SourceObject = Actors[i];
		CueParams.Location = Actors[i]->GetActorLocation();
		CueParams.TargetAttachComponent = BoundPrimary->GetDefaultAttachComponent();
		// Must pass CueParams - the tag-only overload builds fresh params from an empty
		// effect context, leaving GC_ShockLoop with a null TargetAttachComponent.
		// Fork tag: the cue reads this fork actor's location as the beam end, whereas the
		// caster's beam (GameplayCue.ShockLoop) reads the caster's BeamEndLocation.
		TargetASC->AddGameplayCue(FAuraGameplayTags::Get().GameplayCue_ShockLoop_Fork, CueParams);
		AffectedActors.Add(Actors[i]);
	}
	
	return;
}

void UElectrocuteCourse::RemoveEffects()
{
	for (AActor* Affected : AffectedActors)
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Affected))
		{
			ASC->RemoveGameplayCue(FAuraGameplayTags::Get().GameplayCue_ShockLoop_Fork);
		}
	}
	AffectedActors.Empty();
}

void UElectrocuteCourse::DamageTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor)) return;
	if (TargetActor->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(TargetActor)) return;

	FAuraDamageEffectParams Params = MakeDamageEffectParamsFromClassDefaults(TargetActor);
	if (!IsValid(Params.TargetASC) || !IsValid(Params.SourceASC)) return;
	UAuraAbilitySystemLibrary::ApplyDamageEffect(Params);
}

void UElectrocuteCourse::DamageActors()
{
	// Authority only. This ability is LocalPredicted, so the predicting client runs
	// Spawn Electric Beam too - but every effect below is server-owned:
	// AddGameplayCue early-outs unless the owner actor is authoritative (the target's
	// ASC holds no prediction key of ours), and the server's cue then replicates to
	// every client by itself. Running the fork query on the client only burns a sphere
	// overlap and fills AffectedActors with ASCs whose cues it can never remove.
	if (!HasAuthority(&CurrentActivationInfo)) return;
	DamageTarget(BoundPrimary);
	for (AActor* Affected : AffectedActors)
	{
		DamageTarget(Affected);   // DamageTarget already guards validity and IsDead
	}
}

void UElectrocuteCourse::AbilityTick()
{
	RequestCursorTarget();
	if (AAuraCharacterBase* Caster = Cast<AAuraCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Caster->BeamEndLocation = MouseHitLocation;
	}

	// No target: the beam keeps rendering toward BeamEndLocation, there is simply nothing
	// to fork from or damage. UnbindPrimary already dropped the fork cues.
	if (!IsValid(BoundPrimary)) return;

	AffectActors();
	DamageActors();
}

void UElectrocuteCourse::RequestCursorTarget()
{
	if (!IsActive()) return;

	if (TargetDataTask)                  // never leave the previous one running
	{
		TargetDataTask->EndTask();
		TargetDataTask = nullptr;
	}

	TargetDataTask = UTargetDataUnderMouse::CreateTargetDataUnderMouse(this);
	TargetDataTask->ValidData.AddDynamic(this, &UElectrocuteCourse::OnTargetDataReceived);
	TargetDataTask->ReadyForActivation();   // <- without this, Activate() never runs
}

void UElectrocuteCourse::OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data)
{
	const FHitResult Hit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Data, 0);
	if (!Hit.bBlockingHit) return;          // the task always sends, hit or miss
	MouseHitLocation = Hit.ImpactPoint;
	MouseHitActor = Hit.GetActor();
	TraceFirstTarget(MouseHitLocation);
}

void UElectrocuteCourse::RunDamageLogic()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Seed the grace window, or the first tick without a target is already past it
	LastValidTargetTime = World->GetTimeSeconds();

	// Seed the beam end BEFORE adding the cue. The looping timer first fires after one full
	// FChargeTick, so AbilityTick doesn't write BeamEndLocation for that long - but the
	// cue's tick starts reading it on the very next frame, and would get (0,0,0) on a first
	// cast or wherever the previous cast's beam ended.
	if (AAuraCharacterBase* Caster = Cast<AAuraCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Caster->BeamEndLocation = MouseHitLocation;
	}

	// ONE beam cue, hosted on the CASTER for the whole channel. Its endpoint comes from the
	// replicated BeamEndLocation, so sweeping between enemies - or onto bare floor, which
	// has no ASC to host a cue on at all - moves the beam instead of destroying it and
	// respawning it (which restarts the Niagara system and sfx_ShockLoop every switch).
	if (UAbilitySystemComponent* OwnASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters CueParams = FGameplayCueParameters();
		CueParams.SourceObject = GetAvatarActorFromActorInfo();
		CueParams.Location = MouseHitLocation;
		CueParams.TargetAttachComponent = ICombatInterface::Execute_GetWeapon(GetAvatarActorFromActorInfo());
		OwnASC->AddGameplayCue(FAuraGameplayTags::Get().GameplayCue_ShockLoop, CueParams);
	}

	World->GetTimerManager().SetTimer(ChargeTimerHandle, this, &UElectrocuteCourse::AbilityTick, FChargeTick, true);
}

void UElectrocuteCourse::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(ChargeTimerHandle);

	// Counterpart to the caster-hosted beam cue added in RunDamageLogic
	if (UAbilitySystemComponent* OwnASC = GetAbilitySystemComponentFromActorInfo())
	{
		OwnASC->RemoveGameplayCue(FAuraGameplayTags::Get().GameplayCue_ShockLoop);
	}

	SetPrimaryTarget(nullptr);   // unbinds the death delegate and drops the fork cues

	// Undo InShockLoop here rather than only in the BP's PrepToEnd: PrepToEnd runs only on
	// input release, so a cancel/interrupt mid-channel would leave Aura in MOVE_None with
	// the shock-loop pose on for good. Both calls are idempotent if PrepToEnd already ran.
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		if (Avatar->Implements<UCombatInterface>())
		{
			ICombatInterface::Execute_SetInShockLoop(Avatar, false);
		}
		if (ACharacter* AvatarCharacter = Cast<ACharacter>(Avatar))
		{
			AvatarCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
