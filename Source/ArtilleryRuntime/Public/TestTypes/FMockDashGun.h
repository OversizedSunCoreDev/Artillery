 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include <unordered_map>

#include "ArtilleryDispatch.h"
#include "AssetTypeCategories.h"
#include "FGunKey.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "UArtilleryAbilityMinimum.h"
#include "FArtilleryGun.h"
#include "FTLinearVelocity.h"
#include "GameFramework/MovementComponent.h"
#include "FMockDashGun.generated.h"

/**
 * This test class encapsulates a dash.
 * Finally.
 */
USTRUCT(BlueprintType)
struct ARTILLERYRUNTIME_API FMockDashGun : public FArtilleryGun 
{
	GENERATED_BODY()

public:
	// this can be handed into abilities.
	friend class UArtilleryPerActorAbilityMinimum;
	UArtilleryDispatch* MyDispatch;


	//this just sets the gunkey.
	//this mock doesn't use the delegate chaining, because it doesn't use abilities.
	FMockDashGun(const FGunKey& KeyFromDispatch)
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
		UE_LOG(LogTemp, Warning, TEXT("FMockDashGun prefire"));
		auto bind =  MyDispatch->GetAttrib(MyProbableOwner,DASH_CURRENCY);
		if(bind != nullptr && bind->GetCurrentValue() > 1200)
		{
			bind->SetCurrentValue(bind->GetCurrentValue() - 1200.0);
			FireGun(FArtilleryStates::Fired, 0, ActorInfo, ActivationInfo, false, TriggerEventData , Handle);
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
		UE_LOG(LogTemp, Warning, TEXT("FMockDashGun fire"));
		//todo: fix parametrica. RequestAdd should probably set the Anchor. tbh, atm, anchor is always the thread but
		FBLet GameSimPhysicsObject = this->MyDispatch->GetFBLetByObjectKey(MyProbableOwner, this->MyDispatch->GetShadowNow());
		// TODO: Maybe direct this towards movement directional instead of current velocity?
		auto ScaledVelocityContinuation = FBarragePrimitive::GetVelocity(GameSimPhysicsObject).GetSafeNormal() * 1000;
		FTLinearVelocity temp =
			FTLinearVelocity(
				MyProbableOwner,
				VelocityVec(ScaledVelocityContinuation.X, ScaledVelocityContinuation.Y, ScaledVelocityContinuation.Z),
				20
			);

		MyDispatch->RequestAddTicklite(
			MakeShareable(new TL_LinearVelocity(temp)), Early);
		PostFireGun(FArtilleryStates::Fired, 0, ActorInfo, ActivationInfo, false, TriggerEventData, Handle);
	}

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
		UE_LOG(LogTemp, Warning, TEXT("FMockDashGun post"));
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
		MyDispatch = GWorld->GetSubsystem<UArtilleryDispatch>();
		return ARTGUN_MACROAUTOINIT(MyCodeWillHandleKeys);
	}

	FMockDashGun()
	{
		MyGunKey = Default;
	}
	private:
	//Our debug value remains M6D.
	static const inline FGunKey Default = FGunKey("M6D", UINT64_MAX);
};
