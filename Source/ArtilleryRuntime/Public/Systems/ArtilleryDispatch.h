// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UBristleconeWorldSubsystem.h"
#include "UCablingWorldSubsystem.h"
#include "ArtilleryCommonTypes.h"
#include "Containers/TripleBuffer.h"
#include "FArtilleryBusyWorker.h"
#include "LocomotionParams.h"
#include "ConservedAttribute.h"
#include "FArtilleryTicklitesThread.h"
#include "KeyCarry.h"
#include "TransformDispatch.h"
#include "ArtilleryDispatch.generated.h"


/**
 * This is the backbone of artillery, and along with the ArtilleryGuns and UFireControlMachines, 
 * this is most of what any other code will see of artillery in C++. Blueprints will mostly not even see that
 * as they are already encapsulated in the artillerygun's ability sequencer. Abilities merely need to be uninstanced,
 * only modify attributes tracked in Artillery, and use artillery bp nodes where possible.
 * 
 * No unusual additional serialization or similar is required for determinism. I hope.
 * 
 * For an organized view of the code, start by understanding that most work is done outside the gamethread.
 * Bristlecone handles network inputs, Cabling handles keymapping and local inputs. Artillery handles outcomes.
 * 
 * Here's the core rule:
 * 
 * Core gameplay simulation happens in the ArtilleryBusyWorker at about 120hz.
 * Anything that cannot cause the core gameplay sim to become desyncrhonized happens in the game thread.
 * Surprisingly, this means _most things flow through UE normally and happen on the game thread & render threads._
 * 
 * Our goal is that cosmetic ability components, animation, IK, particle systems, cloth, and really, almost everything
 * happens in UE normally. Rollbacks don't happen in the broader UE system at all, and in fact, most of UE acts like its
 * running in a pretty simple Iris push config.
 * 
 * Artillery just pushes transform and ability updates upward as needed. That's a big just, but it really is a narrow scope.
 * 
 * Iris does normal replication on a slow cadence as a fall back and to provide attribute sync reassurances.
 */
struct FArtilleryGun;
namespace Arty
{
	DECLARE_MULTICAST_DELEGATE(OnArtilleryActivated);
	
	DECLARE_DELEGATE_TwoParams(FArtilleryFireGunFromDispatch,
		TSharedPtr<FArtilleryGun> Gun,
		bool InputAlreadyUsedOnce);
	
	//returns true if-and-only-if the duration of the input intent was exhausted.
	DECLARE_DELEGATE_RetVal_FourParams(bool,
		FArtilleryRunLocomotionFromDispatch,
		FArtilleryShell PreviousMovement,
		FArtilleryShell Movement,
		bool RunAtLeastOnce,
		bool Smear);

	typedef FArtilleryTicklitesWorker<UArtilleryDispatch> TickliteWorker;
}

class UCanonicalInputStreamECS;
UCLASS()
class ARTILLERYRUNTIME_API UArtilleryDispatch : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
	friend class FArtilleryBusyWorker;
	friend class FArtilleryTicklitesWorker<UArtilleryDispatch>;
	friend class UCanonicalInputStreamECS;
	friend class UArtilleryLibrary;
protected:
	static inline UArtilleryDispatch* SelfPtr = nullptr;
public:
	
	OnArtilleryActivated BindToArtilleryActivated;
	
	inline ArtilleryTime GetShadowNow()
const
	{
		return ArtilleryAsyncWorldSim.TickliteNow;
	};
	void REGISTER_ENTITY_FINAL_TICK_RESOLVER(ActorKey Self);
	void REGISTER_PROJECTILE_FINAL_TICK_RESOLVER(uint32 MaximumLifespanInTicks, FSkeletonKey Self);
	void REGISTER_GUN_FINAL_TICK_RESOLVER(FGunKey Self);
	void INITIATE_JUMP_TIMER(FSkeletonKey Self);

	//Forwarding for the TickliteThread.
	TOptional<FTransform> GetTransformShadowByObjectKey(FSkeletonKey Target, ArtilleryTime Now)
	{
		if(GetWorld())
		{
			auto TransformECSPillar = GetWorld()->GetSubsystem<UTransformDispatch>();
			if(TransformECSPillar)
			{
				return	TransformECSPillar->CopyOfTransformByObjectKey(Target);
			}
		}
		return TOptional<FTransform>();
	}

	FBLet GetFBLetByObjectKey(FSkeletonKey Target, ArtilleryTime Now)
	{
		if(GetWorld())
		{
			auto PhysicsECSPillar = GetWorld()->GetSubsystem<UBarrageDispatch>();
			if(PhysicsECSPillar)
			{
				return	PhysicsECSPillar->GetShapeRef(Target);
			}
		}
		return FBLet();
	}

	//forwarding for TickliteThread
	//TEMPORARILY REVIVED! HAHAHAHAHA! Speed over elegaerngz
	bool ApplyShadowTransforms()
	{
		if(GetWorld())
		{
			auto TransformECSPillar = GetWorld()->GetSubsystem<UTransformDispatch>();
			if(TransformECSPillar)
			{
				return	TransformECSPillar->ApplyTransformUpdates<TSharedPtr<TransformUpdatesForGameThread>>(TransformUpdateQueue);
			}
		}
		return false;
	}
	
	//Executes necessary preconfiguration for threads owned by this dispatch. Likely going to be factored into the
	//dispatch API so that we can use stronger type guarantees throughout our codebase.
	//Called FROM the thread being set up.
	void ThreadSetup()
	{
		auto PhysicsECSPillar = GetWorld()->GetSubsystem<UBarrageDispatch>();
		if(PhysicsECSPillar)
		{
			PhysicsECSPillar->GrantFeed();
		}
	}

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	TSharedPtr< JOLT::FWorldSimOwner> HoldOpen;
	

	//this is the underlying function mapping we use to queue up Gun Activations.
	//These will eventually need a complete and consistent ordering to ensure determinism.
	//copy op is intentional but may be unneeded. assess before revising signature.
	//TODO: assess if this needs to be a multimap. I think it needs to NOT be.
	TSharedPtr< TMap<FGunKey, FArtilleryFireGunFromDispatch>> GunToFiringFunctionMapping;
	TSharedPtr<BufferedEvents> RequestorQueue_Abilities_TripleBuffer;

	//This is more straightforward than the guns problem.
	//We can actually map this quite directly.
	TSharedPtr< TMap<ActorKey, FArtilleryRunLocomotionFromDispatch>> ActorToLocomotionMapping;
	
	// NOTTODO: It's built!
	TSharedPtr<TMap<FSkeletonKey, AttrMapPtr>> AttributeSetToDataMapping;
	
	TSharedPtr<TMap<FSkeletonKey, IdMapPtr>> IdentSetToDataMapping;
	TSharedPtr<TransformUpdatesForGameThread> TransformUpdateQueue;
public:
	virtual void PostInitialize() override;

protected:

	
	//todo: convert conserved attribute to use a timestamp for versioning to create a true temporal shadowstack.
	AttrMapPtr GetAttribSetShadowByObjectKey(
		FSkeletonKey Target, ArtilleryTime Now) const;
	
	IdMapPtr GetIdSetShadowByObjectKey(
	FSkeletonKey Target, ArtilleryTime Now) const;
	TSharedPtr<BufferedMoveEvents> RequestorQueue_Locomos_TripleBuffer;

	static inline long long TotalFirings = 0; //2024 was rough.
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	FGunKey GetGun(FString GunDefinitionID, ActorKey ProbableOwner);
	//fully specifying the type is necessary to prevent spurious warnings in some cases.
	TSharedPtr<TCircularQueue<std::pair<FGunKey, ArtilleryTime>>> ActionsToOrder;
	//These two are the backbone of the Artillery gun lifecycle.
	TSharedPtr< TMap<FGunKey, TSharedPtr<FArtilleryGun>>> GunByKey;
	TMultiMap<FString, TSharedPtr<FArtilleryGun>> PooledGuns;

	
	/**
	 * Wil hold the configuration for the gun definitions
	 */
	TObjectPtr<UDataTable> GunDefinitionsManifest;
	//TODO: This is a dummy function and should be replaced NLT 10/20/24.
	//It loads from the PROJECT directory. This cannot ship, but will work for all purposes at the moment.
	//Note: https://www.reddit.com/r/unrealengine/comments/160mjkx/how_reliable_and_scalable_are_the_data_tables/
	void LoadGunData();
	
	TSharedPtr<TCircularQueue<std::pair<FGunKey, ArtilleryTime>>> ActionsToReconcile;

	void QueueFire(FGunKey Key, ArtilleryTime Time);

	void QueueResim(FGunKey Key, ArtilleryTime Time);

	//the separation of tick and frame is inspired by the Serious Engine and others.
	//In fact, it's pretty common to this day, with Unity also using a similar model.
	//However, our particular design is running fast relative to most games except quake.
	void RunGuns();
	void RunLocomotions();
	void RunGunFireTimers();
	void CheckFutures();
	//The current start of the tick boundary that ticklites should run on. this allows the ticklites
	//to run in frozen time.
	//********************************
	//DUMMY. USED BY RECONCILE AND RERUN.
	//Here to demonstrate that we actually queue normal and rerun separately.
	//We can't risk intermingling them, which should never happen, but...
	//c'mon. Seriously. you wanna find that bug?
	//********************************
	void RERunGuns();
	void RERunLocomotions();

public:
	typedef FArtilleryTicklitesWorker<UArtilleryDispatch> FTicklitesWorker;
	struct TL_ThreadedImpl 
	{
		//Each class generated gets a unique static. Each kind of dispatcher will get a unique class.
		//TODO: If you run more than one of the parent threads, this gets unsafe. We don't so...
		//As is, it saves a huge amount of memory and indirection costs.
		static inline FTicklitesWorker* ADispatch = nullptr;

		TL_ThreadedImpl()
		{
			if(ADispatch == nullptr)
			{
				throw; // dawg, you tryin' allocate shit against a thread that ain' there.
			}
		}
	
		ArtilleryTime GetShadowNow()
		{
			return ADispatch->GetShadowNow();
		}
	};
	
	//DUMMY FOR NOW.
	//TODO: IMPLEMENT THE GUNMAP FROM INSTANCE UNTO CLASS
	//TODO: REMEMBER TO SAY AMMO A BUNCH
	void RequestAddTicklite(TSharedPtr<TicklitePrototype> ToAdd, TicklitePhase Group)
	{
		ArtilleryTicklitesWorker_LockstepToWorldSim.RequestAddTicklite(ToAdd, Group);
	}
	FGunKey GetGun(FString GunDefinitionID, FireControlKey MachineKey);
	FGunKey RegisterExistingGun(FArtilleryGun* toBind, ActorKey ProbableOwner) const;
	bool ReleaseGun(FGunKey Key, FireControlKey MachineKey);
	
	//TODO: convert to object key to allow the grand dance of the mesh primitives.
	AttrPtr GetAttrib(FSkeletonKey Owner, E_AttribKey Attrib);
	IdentPtr GetIdent(FSkeletonKey Owner, Ident Attrib);
	
	void RegisterReady(FGunKey Key, FArtilleryFireGunFromDispatch Machine)
	{
		GunToFiringFunctionMapping->Add(Key, Machine);
	}
	void RegisterLocomotion(ActorKey Key, FArtilleryRunLocomotionFromDispatch Machine)
	{
		ActorToLocomotionMapping->Add(Key, Machine);
	}
	void Deregister(FGunKey Key)
	{
		auto holdopen = GunToFiringFunctionMapping;
		if(holdopen && holdopen.IsValid())
		{
			GunToFiringFunctionMapping->Remove(Key);
		}
		//TODO: add the rest of the wipe here?
	}
	void RegisterAttributes(FSkeletonKey in, AttrMapPtr Attributes)
	{
		AttributeSetToDataMapping->Add(in, Attributes);
	}
	void RegisterRelationships(FSkeletonKey in, IdMapPtr Relationships)
	{
		IdentSetToDataMapping->Add(in, Relationships);
	}
	void DeregisterAttributes(FSkeletonKey in)
	{
		AttributeSetToDataMapping->Remove(in);
	}
	void DeregisterRelationships(FSkeletonKey in)
	{
		IdentSetToDataMapping->Remove(in);
	}

	std::atomic_bool UseNetworkInput;
	bool missedPrior = false;
	bool burstDropDetected = false;

private:
	static inline long long monotonkey = 0;
	//If you're trying to figure out how artillery works, read the busy worker knowing it's a single thread coming off of Dispatch.
	//this handles input from bristlecone, patching it into streams from the CanonicalInputStreamECS (ACIS), using the ACIS to perform mappings,
	//and processing those streams using the pattern matcher. right now, we also expect it to own rollback and jolt when that's implemented.
	//
	// 
	// This is quite a lot and for performance or legibility reasons there is a good chance that this thread will
	//need to be split up. On the other hand, we want each loop iteration to run for around 2-4 milliseconds.
	//this is quite a bit.
	FArtilleryBusyWorker ArtilleryAsyncWorldSim;
	//This runs the tickables alongside the worldsim, ticking each time the worldsim ticks
	//but NOT using locks. So the processing is totally independent. This is extremely fast,
	//extremely powerful, and the reason why we don't use ticklites when we don't need to.
	//it's dangerous as __________ _____________________ _ _________.
	
	TickliteWorker ArtilleryTicklitesWorker_LockstepToWorldSim;
	TUniquePtr<FRunnableThread> WorldSim_Thread;
	TUniquePtr<FRunnableThread> WorldSim_Ticklites_Thread;
	FSharedEventRef StartTicklitesSim;
	FSharedEventRef StartTicklitesApply;
};

UCLASS(meta=(ScriptName="AbilitySystemLibrary"))
class ARTILLERYRUNTIME_API UArtilleryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, meta = (ScriptName = "GetAttribute", DisplayName = "Get Attribute Of", ExpandBoolAsExecs="bFound"), Category="Artillery|Attributes")
	static float K2_GetAttrib(FSkeletonKey Owner, E_AttribKey Attrib, bool& bFound)
	{
		bFound = false;
		return implK2_GetAttrib(Owner,Attrib, bFound);
	}

	static float implK2_GetAttrib(FSkeletonKey Owner, E_AttribKey Attrib, bool& bFound)
	{
		
		bFound = false;
		if(UArtilleryDispatch::SelfPtr)
		{
			if(UArtilleryDispatch::SelfPtr->GetAttrib( Owner, Attrib))
			{
				bFound = true;
				return UArtilleryDispatch::SelfPtr->GetAttrib( Owner, Attrib)->GetCurrentValue();
			}
		}
		return NAN;
	}
	UFUNCTION(BlueprintCallable, meta = (ScriptName = "GetRelatedKey", DisplayName = "Get Related Key From", ExpandBoolAsExecs="bFound"), Category="Artillery|Keys")
	static FSkeletonKey K2_GetIdentity(FSkeletonKey Owner, E_IdentityAttrib Attrib, bool& bFound)
	{
		
		bFound = false;
		return implK2_GetIdentity(Owner, Attrib, bFound);
	}
	
	static FSkeletonKey implK2_GetIdentity(FSkeletonKey Owner, E_IdentityAttrib Attrib, bool& bFound)
	{
		
		bFound = false;
		if(UArtilleryDispatch::SelfPtr)
		{
			auto ident = UArtilleryDispatch::SelfPtr->GetIdent( Owner, Attrib);
			if(ident)
			{
				bFound = true;
				return ident->CurrentValue;
			}
		}
		return FSkeletonKey();
	}

	UFUNCTION(BlueprintCallable, meta = (ScriptName = "GetPlayerRelatedKey", DisplayName = "Get Local Player's Related Key", WorldContext = "WorldContextObject", HidePin = "WorldContextObject", ExpandBoolAsExecs="bFound"),  Category="Artillery|Keys")
	static FSkeletonKey K2_GetPlayerIdentity(UObject* WorldContextObject, E_IdentityAttrib Attrib, bool& bFound)
	{
		
		bFound = false;
		auto ptr = WorldContextObject->GetWorld()->GetSubsystem<UCanonicalInputStreamECS>();
		if(ptr)
		{
			auto streamkey = ptr->GetStreamForPlayer(PlayerKey::CABLE);
			auto key = ptr->ActorByStream(streamkey);
			if(key)
			{
				return  implK2_GetIdentity(key, Attrib, bFound);
			}
		}
		bFound = false;
		return FSkeletonKey();
	}

	UFUNCTION(BlueprintCallable, meta = (ScriptName = "GetThisActorAttribute", DisplayName = "Get My Actor's Attribute", DefaultToSelf = "Actor", HidePin = "Actor", ExpandBoolAsExecs="bFound"),  Category="Artillery|Attributes")
	static float K2_GetMyAttrib(AActor *Actor, E_AttribKey Attrib, bool& bFound)
	{
	
		bFound = false;
		auto ptr = Actor->GetComponentByClass<UKeyCarry>();
		if(ptr)
		{
			if(FSkeletonKey key = ptr->GetObjectKey())
			{
				return implK2_GetAttrib(key, Attrib, bFound);
			}
		}
		bFound = false;
		return NAN;
	}

	UFUNCTION(BlueprintCallable, meta = (ScriptName = "GetPlayerAttribute", DisplayName = "Get Local Player's Attribute", WorldContext = "WorldContextObject", HidePin = "WorldContextObject", ExpandBoolAsExecs="bFound"),  Category="Artillery|Attributes")
	static float K2_GetPlayerAttrib(UObject* WorldContextObject, E_AttribKey Attrib, bool& bFound)
	{
		bFound = false;
		auto ptr = WorldContextObject->GetWorld()->GetSubsystem<UCanonicalInputStreamECS>();
		if(ptr)
		{
			auto streamkey = ptr->GetStreamForPlayer(PlayerKey::CABLE);
			auto key = ptr->ActorByStream(streamkey);
			if(key)
			{
				return  implK2_GetAttrib(key, Attrib, bFound);
			}
		}
		bFound = false;
		return NAN;
	}
};