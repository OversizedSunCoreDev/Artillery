// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include <unordered_map>

#include "ArtilleryCommonTypes.h"
#include "ArtilleryProjectileDispatch.h"
#include "FAttributeMap.h"
#include "FGunKey.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "UArtilleryAbilityMinimum.h"
#include "Camera/CameraComponent.h"
#include "FArtilleryGun.generated.h"


/**
 * * GUNS MUST BE INITIALIZED. This is handled in the various loaders and builders, but any unique gun MUST be initialized.
 * This class will be a data-driven instance of a gun that encapsulates a generic structured ability,
 * then exposes bindings for the phases of that ability as a component to be bound as specific gameplay abilities.
 *
 * 
 * Artillery gun is a not a UObject. This allows us to safely interact with it off the game thread TO AN EXTENT.
 *
 * Ultimately, we'll need something like https://github.com/facebook/folly/blob/main/folly/concurrency/ConcurrentHashMap.h
 * if we want to get serious about this.
 *
 */
USTRUCT(BlueprintType)
struct ARTILLERYRUNTIME_API FArtilleryGun
{
	GENERATED_BODY()

public:
	// this can be handed into abilities.
	friend class UArtilleryPerActorAbilityMinimum;
	FGunKey MyGunKey;
	ActorKey MyProbableOwner;
	bool ReadyToFire = false;

	UArtilleryDispatch* MyDispatch;
	UArtilleryProjectileDispatch* MyProjectileDispatch;
	TSharedPtr<FAttributeMap> MyAttributes;

	// Owner Components
	TWeakObjectPtr<UCameraComponent> PlayerCameraComponent;
	TWeakObjectPtr<USceneComponent> FiringPointComponent;

	// 0 MaxAmmo = No Ammo system required
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int MaxAmmo = 30;
	// Frames to cooldown and fire again
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Firerate = 50;
	// Frames to reload
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int ReloadTime = 120;

	//As these are UProperties, they should NOT need to become strong pointers or get attached to root
	//to _exist_ when created off main thread, but that doesn't solve the bulk of the issues and the guarantee
	//hasn't held up as well as I would like.


	//these need to be added to the rootset to prevent GC erasure. UProperty isn't enough alone, as this class
	//is not GC reachable. This means that as soon as the reference expires and the sweep completes, as is, you'll
	//get an error. There are two good solutions: one is keeping a bank of abilities in an ECS component or fire control
	//machines. I don't love the idea of fire control machines owning abilities directly, but it is idiomatic within
	//gas. For the time being, we simply add them to Root, but this is highly undesirable. On the other hand, it's easy to change
	//and does not commit us to an architecture at the moment.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UArtilleryPerActorAbilityMinimum> Prefire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UArtilleryPerActorAbilityMinimum> PrefireCosmetic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UArtilleryPerActorAbilityMinimum> Fire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UArtilleryPerActorAbilityMinimum> FireCosmetic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UArtilleryPerActorAbilityMinimum> PostFire;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UArtilleryPerActorAbilityMinimum> PostFireCosmetic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UArtilleryPerActorAbilityMinimum> FailedFireCosmetic;

	//we use the GunBinder delegate to link the MECHANICAL abilities to phases.
	//cosmetics don't get linked the same way.
	FArtilleryGun(const FGunKey& KeyFromDispatch)
	{
		MyDispatch = nullptr;
		MyProjectileDispatch = nullptr;
		MyGunKey = KeyFromDispatch;
	};

	virtual ~FArtilleryGun()
	{
		if(Prefire != nullptr) //we always assign all or none, so we can just check prefire atm. this might change.
		{
			Prefire->RemoveFromRoot();
			Fire->RemoveFromRoot();
			PostFire->RemoveFromRoot();
			PrefireCosmetic->RemoveFromRoot();
			FireCosmetic->RemoveFromRoot();
			PostFireCosmetic->RemoveFromRoot();
			FailedFireCosmetic->RemoveFromRoot();
		}
	}

	void UpdateProbableOwner(ActorKey ProbableOwner)
	{
		MyProbableOwner = ProbableOwner;
	}
	//I'm sick and tired of the endless layers of abstraction.
	//Here's how it works. we fire the abilities from the gun.
	//OnGameplayAbilityEnded doesn't actually let you know if the ability was canceled.
	//That's... not so good. We use OnGameplayAbilityEndedWithData instead.
	virtual void PreFireGun(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData = nullptr,
		bool RerunDueToReconcile = false,
		int DallyFramesToOmit = 0)
	{
		// Delegate type:
		// DECLARE_DELEGATE_FiveParams FArtilleryAbilityStateAlert
		Prefire->GunBinder.BindRaw(this, &FArtilleryGun::FireGun, RerunDueToReconcile, TriggerEventData, Handle);
		Fire->GunBinder.BindRaw(this, &FArtilleryGun::PostFireGun, RerunDueToReconcile, TriggerEventData, Handle);
		Prefire->CallActivateAbility(FGameplayAbilitySpecHandle(), ActorInfo, ActivationInfo, nullptr,
		                             TriggerEventData);
		if (!RerunDueToReconcile)
		{
			PrefireCosmetic->CallActivateAbility(Handle, ActorInfo, ActivationInfo, nullptr, TriggerEventData);
		}
	};

	/**************************************
	 *the following are delegates for Ability Minimum.
	 * We could likely use this with some cleverness to avoid object alloc while still getting
	 * per instance behavior but atm, that's not something I'm building.
	 * Dally frames don't work yet. But they will. Be ready.
	 *
	 * These are fired during end ability.
	 *****************************************
	 */

	/* This fires the gun when the prefire ability succeeds.
 * It will be tempting to reorder the parameters. Don't do this.
 * Again, this is a delegate function and the parametric order is what enables payloading.
 */
	virtual void FireGun(
		FArtilleryStates OutcomeStates,
		int DallyFramesToOmit,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool RerunDueToReconcile,
		const FGameplayEventData* TriggerEventData,
		FGameplayAbilitySpecHandle Handle)
	{
		if(!ReadyToFire)
		{
			throw; //your gun is broken. if you don't like this, override this function.
		}
		if (OutcomeStates == FArtilleryStates::Fired)
		{
			Fire->CallActivateAbility(FGameplayAbilitySpecHandle(), ActorInfo, ActivationInfo, nullptr,
			                          TriggerEventData);
			//TODO: BUILD CORRECT HANDLE HANDLING. HANDLES ARE OUR TICKET OUT OF THIS JOINT.
			if (!RerunDueToReconcile)
			{
				//TODO: BUILD CORRECT HANDLE HANDLING. HANDLES ARE OUR TICKET OUT OF THIS JOINT.
				FireCosmetic->CallActivateAbility(FGameplayAbilitySpecHandle(), ActorInfo, ActivationInfo, nullptr,
				                                  TriggerEventData);
			}
		}
		else
		{
			if (!RerunDueToReconcile)
			{
				//TODO: BUILD CORRECT HANDLE HANDLING. HANDLES ARE OUR TICKET OUT OF THIS JOINT.
				FailedFireCosmetic->CallActivateAbility(FGameplayAbilitySpecHandle(), ActorInfo, ActivationInfo,
				                                        nullptr, TriggerEventData);
			}
		}
	};

	virtual void PostFireGun(
		FArtilleryStates OutcomeStates,
		int DallyFramesToOmit,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool RerunDueToReconcile,
		const FGameplayEventData* TriggerEventData,
		FGameplayAbilitySpecHandle Handle) 
	{
		if (OutcomeStates == FArtilleryStates::Fired)
		{
			PostFire->CallActivateAbility(FGameplayAbilitySpecHandle(), ActorInfo, ActivationInfo, nullptr,
			                              TriggerEventData);
			//TODO: BUILD CORRECT HANDLE HANDLING. HANDLES ARE OUR TICKET OUT OF THIS JOINT.
			if (!RerunDueToReconcile)
			{
				//TODO: BUILD CORRECT HANDLE HANDLING. HANDLES ARE OUR TICKET OUT OF THIS JOINT.
				PostFireCosmetic->CallActivateAbility(FGameplayAbilitySpecHandle(), ActorInfo, ActivationInfo, nullptr,
				                                      TriggerEventData);
			}
		}
		else
		{
			if (!RerunDueToReconcile)
			{
				//TODO: BUILD CORRECT HANDLE HANDLING. HANDLES ARE OUR TICKET OUT OF THIS JOINT.
				FailedFireCosmetic->CallActivateAbility(FGameplayAbilitySpecHandle(), ActorInfo, ActivationInfo,
				                                        nullptr, TriggerEventData);
			}
		}
	};

	//The unusual presence of the modal switch AND a requirement for the related parameter is due to the
	//various fun vagaries of inheritance. IF you override this function, and any valid child class should,
	//then you'll want to have some assurance of fine-grained control there over all your parent classes.
	//for a variety of reasons, a gunkey might not be null, but might not be usable or desirable.
	//please ensure your child classes respect this as well. thank you!
	//returns readytofire
#define ARTGUN_MACROAUTOINIT(MyCodeWillHandleKeys) Super::Initialize(KeyFromDispatch, MyCodeWillHandleKeys, PF, PFC,F,FC,PtF,PtFc,FFC)
	virtual bool Initialize(
		const FGunKey& KeyFromDispatch,
		const bool MyCodeWillSetGunKey,
		UArtilleryPerActorAbilityMinimum* PF = nullptr,
		UArtilleryPerActorAbilityMinimum* PFC = nullptr,
		UArtilleryPerActorAbilityMinimum* F = nullptr,
		UArtilleryPerActorAbilityMinimum* FC = nullptr,
		UArtilleryPerActorAbilityMinimum* PtF = nullptr,
		UArtilleryPerActorAbilityMinimum* PtFc = nullptr,
		UArtilleryPerActorAbilityMinimum* FFC = nullptr
		)
	{
		MyGunKey = KeyFromDispatch;
		//assign gunkey
		
		MyDispatch = GWorld->GetSubsystem<UArtilleryDispatch>();
		MyProjectileDispatch = GWorld->GetSubsystem<UArtilleryProjectileDispatch>();

		TMap<AttribKey, double> InitialGunAttributes = TMap<AttribKey, double>();
		// TODO: load more stats and dynamically rather than fixed demo values
		InitialGunAttributes.Add(AMMO, MaxAmmo);
		InitialGunAttributes.Add(MAX_AMMO, MaxAmmo);
		InitialGunAttributes.Add(COOLDOWN, Firerate);
		InitialGunAttributes.Add(COOLDOWN_REMAINING, 0);
		InitialGunAttributes.Add(RELOAD, ReloadTime);
		InitialGunAttributes.Add(RELOAD_REMAINING, 0);
		InitialGunAttributes.Add(TICKS_SINCE_GUN_LAST_FIRED, 0);
		InitialGunAttributes.Add(AttribKey::LastFiredTimestamp, 0);
		MyAttributes = MakeShareable(new FAttributeMap(MyGunKey, MyDispatch, InitialGunAttributes));
		MyDispatch->REGISTER_GUN_FINAL_TICK_RESOLVER(MyGunKey);

		UTransformDispatch* TransformDispatch = MyDispatch->GetWorld()->GetSubsystem<UTransformDispatch>();
		TWeakObjectPtr<AActor> ActorPointer = TransformDispatch->GetAActorByObjectKey(MyProbableOwner);
		check(ActorPointer.IsValid());
PlayerCameraComponent
		 = ActorPointer->GetComponentByClass<UCameraComponent>();
		FiringPointComponent = Cast<USceneComponent, UObject>(ActorPointer->GetDefaultSubobjectByName(TEXT("WeaponFiringPoint")));
		
		//we'd like to do it earlier, but there's actually not a great moment to do this.
		if(Prefire == nullptr)
		{
			Prefire = PF ? PF : NewObject<UArtilleryPerActorAbilityMinimum>();
			Prefire->AddToRoot();
			Fire = F ? F :	NewObject<UArtilleryPerActorAbilityMinimum>();
			Fire->AddToRoot();
			PostFire = PtF ? PtF : NewObject<UArtilleryPerActorAbilityMinimum>();
			PostFire->AddToRoot();
			
			PrefireCosmetic  = PFC? PFC : NewObject<UArtilleryPerActorAbilityMinimum>();
			PrefireCosmetic->AddToRoot();
			FireCosmetic = FC ? FC : NewObject<UArtilleryPerActorAbilityMinimum>();
			FireCosmetic->AddToRoot();
			PostFireCosmetic = PtFc ? PtFc : NewObject<UArtilleryPerActorAbilityMinimum>();
			PostFireCosmetic->AddToRoot();
			FailedFireCosmetic = FFC ? FFC : NewObject<UArtilleryPerActorAbilityMinimum>();
			FailedFireCosmetic->AddToRoot();
		}
		if(!MyCodeWillSetGunKey)
		{
			SetGunKey(MyGunKey);
		}
		ReadyToFire = ReadyToFire || !MyCodeWillSetGunKey;
		return ReadyToFire;
	}

	void SetGunKey(FGunKey NewKey) const
	{
		Prefire->MyGunKey = MyGunKey;
		PrefireCosmetic->MyGunKey = MyGunKey;
		Fire->MyGunKey = MyGunKey;
		FireCosmetic->MyGunKey = MyGunKey;
		PostFire->MyGunKey = MyGunKey;
		PostFireCosmetic->MyGunKey = MyGunKey;
		FailedFireCosmetic->MyGunKey = MyGunKey;
	}
	FArtilleryGun()
	{
		MyGunKey = Default;
	}


private:
	//Our debug value remains M6D.
	static const inline FGunKey Default = FGunKey("M6D", UINT64_MAX);
};
