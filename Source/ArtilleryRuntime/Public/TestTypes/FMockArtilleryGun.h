 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include <unordered_map>
#include "FGunKey.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "UArtilleryAbilityMinimum.h"
#include "FArtilleryGun.h"
#include "FMockArtilleryGun.generated.h"




/**
 * This class will be a data-driven instance of a gun that encapsulates a generic structured ability,
 * then exposes bindings for the phases of that ability as a component to be bound as specific gameplay abilities.
 *
 * 
 * Artillery gun is a not a UObject. This allows us to safely interact with it off the game thread.
 * Triggering the abilities is likely a no-go off the thread, but we can modify the attributes as needed.
 * This allows us to do some very powerful stuff to ensure that we always have the most up to date data.
 * Some dark things.
 *
 * Ultimately, we'll need something like https://github.com/facebook/folly/blob/main/folly/concurrency/ConcurrentHashMap.h
 * if we want to get serious about this.
 */
USTRUCT(BlueprintType)
struct ARTILLERYRUNTIME_API FMockArtilleryGun : public FArtilleryGun 
{
	GENERATED_BODY()

public:
	// this can be handed into abilities.
	friend class UArtilleryPerActorAbilityMinimum;

	//this just sets the gunkey.
	//this mock doesn't use the delegate chaining, because it doesn't use abilities.
	FMockArtilleryGun(const FGunKey& KeyFromDispatch)
	{
		MyGunKey = KeyFromDispatch;
	};


	virtual void PreFireGun(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData = nullptr,
		bool RerunDueToReconcile = false,
		int DallyFramesToOmit = 0) 
		override
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, FString::Printf(TEXT("Mock Gun Fired. Is reconcile? %hs"), RerunDueToReconcile ? "true" : "false"));
		}
	};

	virtual void FireGun(
		FArtilleryStates OutcomeStates,
		int DallyFramesToOmit,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool RerunDueToReconcile,
		const FGameplayEventData* TriggerEventData,
		FGameplayAbilitySpecHandle Handle) 
		override
	{
		throw;
	};

	virtual void PostFireGun(
		FArtilleryStates OutcomeStates,
		int DallyFramesToOmit,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool RerunDueToReconcile,
		const FGameplayEventData* TriggerEventData,
		FGameplayAbilitySpecHandle Handle) 
		override
	{
			throw;
	};
	
	virtual bool Initialize(
		const FGunKey& KeyFromDispatch,
		const bool MyCodeWillHandleKeys,
		UArtilleryPerActorAbilityMinimum* PF = nullptr,
		UArtilleryPerActorAbilityMinimum* PFC = nullptr,
		UArtilleryPerActorAbilityMinimum* F = nullptr,
		UArtilleryPerActorAbilityMinimum* FC = nullptr,
		UArtilleryPerActorAbilityMinimum* PtF = nullptr,
		UArtilleryPerActorAbilityMinimum* PtFc = nullptr,
		UArtilleryPerActorAbilityMinimum* FFC = nullptr)
		override
	{
		return ARTGUN_MACROAUTOINIT(MyCodeWillHandleKeys);
	}

	FMockArtilleryGun()
	{
		MyGunKey = Default;
	}
	private:
	//Our debug value remains M6D.
	static const inline FGunKey Default = FGunKey("M6D", UINT64_MAX);


};
