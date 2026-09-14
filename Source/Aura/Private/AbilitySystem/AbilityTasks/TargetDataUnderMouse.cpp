 // Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/TargetDataUnderMouse.h"
#include "AbilitySystemComponent.h"
#include "Aura/Aura.h"

 UTargetDataUnderMouse* UTargetDataUnderMouse::CreateTargetDataUnderMouse(UGameplayAbility* OwningAbility)
{
	UTargetDataUnderMouse* MyObj = NewAbilityTask<UTargetDataUnderMouse>(OwningAbility);

	return MyObj;
}

void UTargetDataUnderMouse::Activate()
{
	const bool bIsLocallyControlled = Ability->GetCurrentActorInfo()->IsLocallyControlled();
	if (bIsLocallyControlled)
	{
		//Local player: send mouse click location
		SendMouseCursorData();
	}
	else
	{
		//Server, receive local player data
		const FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
		const FPredictionKey ActivationPredictionKey = GetActivationPredictionKey();
		AbilitySystemComponent.Get()->AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey).AddUObject(
			this, &UTargetDataUnderMouse::OnTargetDataReplicatedCallback);
		
		const bool bCalledDelegate = AbilitySystemComponent.Get()->CallReplicatedTargetDataDelegatesIfSet(SpecHandle, ActivationPredictionKey);
		if (!bCalledDelegate)
		{
			SetWaitingOnRemotePlayerData();
		}
	}
}

void UTargetDataUnderMouse::SendMouseCursorData()
{
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get());

	auto PlayerController = Ability->GetCurrentActorInfo()->PlayerController.Get();
	FHitResult CursorHit;
	if (PlayerController)
	{
		PlayerController->GetHitResultUnderCursor(ECC_Target, false, CursorHit);
	}
	// Always send, even without a blocking hit: returning early here leaves the
	// server-side task in SetWaitingOnRemotePlayerData forever, so the ability never
	// ends and its input slot stays dead. Receivers must check bBlockingHit instead.

	FGameplayAbilityTargetDataHandle DataHandle;
	FGameplayAbilityTargetData_SingleTargetHit* Data = new FGameplayAbilityTargetData_SingleTargetHit();
	
	Data->HitResult = CursorHit;
	DataHandle.Add(Data);
	AbilitySystemComponent->ServerSetReplicatedTargetData(
		GetAbilitySpecHandle(),
		GetActivationPredictionKey(),
		DataHandle,
		FGameplayTag(),
		AbilitySystemComponent->ScopedPredictionKey);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(DataHandle);
	}
	EndTask();   // one sample per task, see OnTargetDataReplicatedCallback
}

void UTargetDataUnderMouse::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	// One sample per task. The target-data delegate is keyed by the ACTIVATION, not by the task,
	// so a server task that stays bound fires again for every later sample of the same activation -
	// including the channel aim loop's 20 per second. For the Blueprint's activation-time task that
	// re-ran its whole ValidData chain: replaying the cast montage interrupted the previous
	// PlayMontageAndWait (Meteorite: OnInterrupted -> EndAbility), and Electrocute re-committed and
	// restarted its damage timer on every sample. Listen-server hosts never bind, so only clients saw it.
	//
	// Copy BEFORE consuming. When the sample was cached before this task existed, DataHandle is a
	// reference into the ASC's cache (CallReplicatedTargetDataDelegatesIfSet), and consuming clears
	// it - the broadcast then carried an empty handle, i.e. a hit with bBlockingHit=false. The copy
	// shares the target data's pointers, so it survives the clear.
	const FGameplayAbilityTargetDataHandle Data = DataHandle;
	AbilitySystemComponent->AbilityTargetDataSetDelegate(GetAbilitySpecHandle(), GetActivationPredictionKey()).RemoveAll(this);
	AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(Data);
	}
	EndTask();
}
