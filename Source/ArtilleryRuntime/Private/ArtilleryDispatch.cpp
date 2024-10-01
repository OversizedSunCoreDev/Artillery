// Fill out your copyright notice in the Description page of Project Settings.


#include "ArtilleryDispatch.h"
#include "FArtilleryGun.h"
#include <FTEntityFinalTickResolver.h>
#include <FTGunFinalTickResolver.h>
#include <FTJumpTimer.h>

#include "FTProjectileFinalTickResolver.h"


//Place at the end of the latest initialization-like phase.
//should we move this lil guy over into ya boy Dispatch? It feels real dispatchy.
void UArtilleryDispatch::REGISTER_ENTITY_FINAL_TICK_RESOLVER(ActorKey Self)
{
	TLEntityFinalTickResolver temp = TLEntityFinalTickResolver(Self); //this semantic sucks. gotta fix it.
	this->RequestAddTicklite(MakeShareable(new EntityFinalTickResolver(temp)), FINAL_TICK_RESOLVE);
}

void UArtilleryDispatch::REGISTER_PROJECTILE_FINAL_TICK_RESOLVER(uint32 MaximumLifespanInTicks, FSkeletonKey Self)
{
	TLProjectileFinalTickResolver temp = TLProjectileFinalTickResolver(MaximumLifespanInTicks, Self);
	this->RequestAddTicklite(MakeShareable(new ProjectileFinalTickResolver(temp)), FINAL_TICK_RESOLVE);
}

void UArtilleryDispatch::REGISTER_GUN_FINAL_TICK_RESOLVER(FGunKey Self)
{
	TLGunFinalTickResolver temp = TLGunFinalTickResolver(Self); //this semantic sucks. gotta fix it.
	this->RequestAddTicklite(MakeShareable(new GunFinalTickResolver(temp)), FINAL_TICK_RESOLVE);
}

void UArtilleryDispatch::INITIATE_JUMP_TIMER(FSkeletonKey Self)
{
	FTJumpTimer JumpTimer = FTJumpTimer(Self);
	this->RequestAddTicklite(MakeShareable(new TL_JumpTimer(JumpTimer)), Normal);
}

void UArtilleryDispatch::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("ArtilleryDispatch:Subsystem: Online"));
	RequestorQueue_Abilities_TripleBuffer = MakeShareable( new TTripleBuffer<TArray<TPair<BristleTime,FGunKey>>>());
	RequestorQueue_Locomos_TripleBuffer = MakeShareable( new TTripleBuffer<TArray<LocomotionParams>>());
	GunToFiringFunctionMapping = MakeShareable(new TMap<FGunKey, FArtilleryFireGunFromDispatch>());
	ActorToLocomotionMapping = MakeShareable(new TMap<ActorKey, FArtilleryRunLocomotionFromDispatch>());
	AttributeSetToDataMapping = MakeShareable( new TMap<FSkeletonKey, AttrMapPtr>());
	IdentSetToDataMapping = MakeShareable(new TMap<FSkeletonKey, IdMapPtr>());
	GunByKey = MakeShareable(new TMap<FGunKey, TSharedPtr<FArtilleryGun>>());
	TL_ThreadedImpl::ADispatch = &ArtilleryTicklitesWorker_LockstepToWorldSim;
	SelfPtr = this;
}

void UArtilleryDispatch::PostInitialize()
{
	Super::PostInitialize();
	UBarrageDispatch* PhysicsECS = GetWorld()->GetSubsystem<UBarrageDispatch>();
	TransformUpdateQueue = PhysicsECS->GameTransformPump;
	
	UCanonicalInputStreamECS* InputECS = GetWorld()->GetSubsystem<UCanonicalInputStreamECS>();
	ArtilleryAsyncWorldSim.CablingControlStream = InputECS->getNewStreamConstruct(APlayer::CABLE);
	ArtilleryAsyncWorldSim.BristleconeControlStream = InputECS->getNewStreamConstruct(APlayer::ECHO);
}

void UArtilleryDispatch::OnWorldBeginPlay(UWorld& InWorld)
{
	
	if ([[maybe_unused]] const UWorld* World = InWorld.GetWorld()) {
		UE_LOG(LogTemp, Warning, TEXT("ArtilleryDispatch:Subsystem: World beginning play"));
		// getting input from Bristle
		UseNetworkInput.store(true);
		UBristleconeWorldSubsystem* NetworkAndControls = GetWorld()->GetSubsystem<UBristleconeWorldSubsystem>();
		UBarrageDispatch* GameSimPhysics = GetWorld()->GetSubsystem<UBarrageDispatch>();
		HoldOpen = GameSimPhysics->JoltGameSim;
		ArtilleryTicklitesWorker_LockstepToWorldSim.DispatchOwner = this;
		ArtilleryTicklitesWorker_LockstepToWorldSim.StartTicklitesApply = StartTicklitesApply;
		ArtilleryTicklitesWorker_LockstepToWorldSim.StartTicklitesSim = StartTicklitesSim;
		ArtilleryAsyncWorldSim.StartTicklitesApply = StartTicklitesApply;
		ArtilleryAsyncWorldSim.StartTicklitesSim = StartTicklitesSim;
		ArtilleryAsyncWorldSim.InputRingBuffer = MakeShareable(new PacketQ(256));
		NetworkAndControls->QueueOfReceived = ArtilleryAsyncWorldSim.InputRingBuffer;
		UCablingWorldSubsystem* DirectLocalInputSystem = GetWorld()->GetSubsystem<UCablingWorldSubsystem>();
		ArtilleryAsyncWorldSim.InputSwapSlot = MakeShareable(new IncQ(256));
		DirectLocalInputSystem->DestructiveChangeLocalOutboundQueue(ArtilleryAsyncWorldSim.InputSwapSlot);
		UCanonicalInputStreamECS* InputStreamECS = GetWorld()->GetSubsystem<UCanonicalInputStreamECS>();
		ArtilleryAsyncWorldSim.ContingentInputECSLinkage = InputStreamECS;
		ArtilleryAsyncWorldSim.ContingentPhysicsLinkage = GameSimPhysics;
		//IF YOU REMOVE THIS. EVERYTHING EXPLODE. IN A BAD WAY.
		//TARRAY IS A VALUE TYPE. SO IS TRIPLEBUFF I THINK.
		ArtilleryAsyncWorldSim.RequestorQueue_Abilities_TripleBuffer = RequestorQueue_Abilities_TripleBuffer;//OH BOY. REFERENCE TIME. GWAHAHAHA.
		ArtilleryAsyncWorldSim.RequestorQueue_Locomos_TripleBuffer = RequestorQueue_Locomos_TripleBuffer;
		UBarrageDispatch* PhysicsECS = GetWorld()->GetSubsystem<UBarrageDispatch>();
		PhysicsECS->GrantFeed();
		
		WorldSim_Thread.Reset(FRunnableThread::Create(&ArtilleryAsyncWorldSim, TEXT("ARTILLERY_ONLINE.")));
		WorldSim_Ticklites_Thread.Reset(FRunnableThread::Create(&ArtilleryTicklitesWorker_LockstepToWorldSim ,TEXT("BARRAGE_ONLINE.")));


	}
	
}

void UArtilleryDispatch::Deinitialize()
{

	Super::Deinitialize();
	StartTicklitesSim->Trigger();
	ArtilleryTicklitesWorker_LockstepToWorldSim.running = false;
	ArtilleryAsyncWorldSim.Stop();
	StartTicklitesApply->Trigger();
	ArtilleryTicklitesWorker_LockstepToWorldSim.Stop();
	//We have to wait on worldsim, but we actually can just hard kill ticklites.
	if(WorldSim_Thread.IsValid())
	{
		//if we don't wait, this will crash when the truth of the matter is referenced. That's just the facts.
		WorldSim_Thread->Kill(true);
	}
	if(WorldSim_Ticklites_Thread.IsValid())
	{
		//otoh, we need to hard kill the ticklites, so far as I can tell, and keep rolling. This should actually generally
		//not proc.
		WorldSim_Ticklites_Thread->Kill(false);
	}
	AttributeSetToDataMapping->Empty();
	IdentSetToDataMapping->Empty();
	GunToFiringFunctionMapping->Empty();
	ActorToLocomotionMapping->Empty();
	HoldOpen.Reset();
}





AttrMapPtr UArtilleryDispatch::GetAttribSetShadowByObjectKey(FSkeletonKey Target,
	ArtilleryTime Now) const
{
	return AttributeSetToDataMapping->FindChecked(Target);
}

IdMapPtr UArtilleryDispatch::GetIdSetShadowByObjectKey(FSkeletonKey Target,
	ArtilleryTime Now) const
{
	return IdentSetToDataMapping->FindChecked(Target);
}

void UArtilleryDispatch::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RunGuns(); // ALL THIS WORK. FOR THIS?! (Okay, that's really cool)

	auto PhysicsECSPillar = GetWorld()->GetSubsystem<UBarrageDispatch>();
	if(PhysicsECSPillar)
	{
		auto TransformECSPillar = GetWorld()->GetSubsystem<UTransformDispatch>();
		if(TransformECSPillar)
		{
			//because we are on the game thread, we can get away without full hold opens for now.
			//I still want to refactor this into the threads somehow, but I just don't see a way right now
			//since this calls gt-locked functions on actors in a lot of places.
			TransformECSPillar->ApplyTransformUpdates
			<TSharedPtr<TransformUpdatesForGameThread>>
			(PhysicsECSPillar->GameTransformPump);
		}
		PhysicsECSPillar->BroadcastContactEvents();
	}
}

TStatId UArtilleryDispatch::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UArtilleryDispatch, STATGROUP_Tickables);
}


FGunKey UArtilleryDispatch::GetGun(FString GunDefinitionID, ActorKey ProbableOwner)
{
	//We know it. We have known it. We continue to know it.
	//See you soon, Chief.
	GunDefinitionID = GunDefinitionID.IsEmpty() ? "M6D" : GunDefinitionID; //joking aside, an obvious debug val is needed.
	FGunKey Key = FGunKey(GunDefinitionID, monotonkey++);
	TMap<AttribKey, double> InitialGunAttributes = TMap<AttribKey, double>();
	if(PooledGuns.Contains(GunDefinitionID))
	{
		TSharedPtr<FArtilleryGun> repurposing = *PooledGuns.Find(GunDefinitionID);
		PooledGuns.RemoveSingle(GunDefinitionID, repurposing);
		repurposing->Initialize(Key, false);
		repurposing->UpdateProbableOwner(ProbableOwner);
		GunByKey->Add(Key, repurposing);
	}
	else
	{
		TSharedPtr<FArtilleryGun> NewGun = MakeShareable(new FArtilleryGun(Key));
		NewGun->Initialize(Key, false);
		NewGun->UpdateProbableOwner(ProbableOwner);
		GunByKey->Add(Key, NewGun);
	}
	return Key;	
}

FGunKey UArtilleryDispatch::RegisterExistingGun(FArtilleryGun* ToBind, ActorKey ProbableOwner) const
{
	//TODO: see if this code path needs to evolve to do more sophisticated management of the gunkey itself
	TSharedPtr<FArtilleryGun> NewGun = MakeShareable(ToBind);
	NewGun->UpdateProbableOwner(ProbableOwner);
	GunByKey->Add(ToBind->MyGunKey, NewGun);
	return ToBind->MyGunKey;	
}

//returns false if already released.
bool UArtilleryDispatch::ReleaseGun(FGunKey Key, FireControlKey MachineKey)
{
	//We know it. We have known it. We continue to know it.
	//See you soon, Chief.
	if(GunByKey->Contains(Key))
	{
		
		TSharedPtr<FArtilleryGun> tracker;
		GunByKey->RemoveAndCopyValue(Key, tracker);
		PooledGuns.Add(Key.GunDefinitionID, tracker);
		return true;
	}
	return false;	
}

void UArtilleryDispatch::QueueResim(FGunKey Key, ArtilleryTime Time)
{
	if (ActionsToReconcile && ActionsToReconcile.IsValid())
	{
		ActionsToReconcile->Enqueue(std::pair<FGunKey, ArtilleryTime>(Key, Time));
	}
}

AttrPtr UArtilleryDispatch::GetAttrib(FSkeletonKey Owner, AttribKey Attrib)
{
		if(AttributeSetToDataMapping->Contains(Owner))
		{
			auto a = AttributeSetToDataMapping->FindChecked(Owner);
			if(a->Contains(Attrib))
			{
				return a->FindChecked(Attrib);
			}
		}
		return nullptr;
}

IdentPtr UArtilleryDispatch::GetIdent(FSkeletonKey Owner, Ident Attrib)
{
	if(IdentSetToDataMapping->Contains(Owner))
	{
		auto a = IdentSetToDataMapping->FindChecked(Owner);
		if(a->Contains(Attrib))
		{
			return a->FindChecked(Attrib);
		}
	}
	return nullptr;
}



void UArtilleryDispatch::RunGuns()
{

	
	if(RequestorQueue_Abilities_TripleBuffer && RequestorQueue_Abilities_TripleBuffer->IsDirty())
	//Sort is not stable. Sortedness appears to be lost for operations I would not expect.
	{
		RequestorQueue_Abilities_TripleBuffer->SwapReadBuffers();
		for (auto x : RequestorQueue_Abilities_TripleBuffer->Read())
		{
			auto fired =  GunToFiringFunctionMapping->Find(x.Value)->ExecuteIfBound(
				GunByKey->FindRef(x.Value)
				, false);
			TotalFirings += fired;
		}
		RequestorQueue_Abilities_TripleBuffer->Read().Reset();
	}
}

//this needs work and extension.
//TODO: add smear support.
void UArtilleryDispatch::RunLocomotions()
{
	if(RequestorQueue_Locomos_TripleBuffer->IsDirty())
	{
		RequestorQueue_Locomos_TripleBuffer->SwapReadBuffers();
		//Sort is not stable. Sortedness appears to be lost for operations I would not expect.
		for (auto x : RequestorQueue_Locomos_TripleBuffer->Read())
		{
			//execute if bound cannot be used with return values
			//because Unreal does not use the STL or did not when that code was written
			//so they don't have the easy elegant idiom of the Optional as readily.
			bool fired = ActorToLocomotionMapping->Find(x.parent)->
			Execute(
				 x.previousIndex,
				 x.currentIndex,
				 false,
				 false
				 );
			TotalFirings += fired;
		}
		RequestorQueue_Locomos_TripleBuffer->Read().Reset();
	}
}



void UArtilleryDispatch::RunGunFireTimers()
{

}

//this peeks the various queues of things to do in the future, such as the velocity queue or the gun timer queues
//it also checks the local sorted list, allowing us to manage timers in amortized log(n) time in the worst case.
//normally a design like this isn't practical, but since we can allocate a thread to the process of maintaining the sort
//and only push events ready to go, there's only one thread touching the sorted sets, allowing us to go totally lockfree
//in exchange for some extra copy ops that we'd have needed to incur anyway to allow data shadowing.
//this is one of the huge advantages to the data shadowing scheme, namely, it turns weakness into strength.
//this always gets called from the busy worker, and populates the velocity and gun events stacks. eventually, those
//will be obsoleted to some extent and at least some of the events can be run immediately on the artillery busy worker thread
//thanks again to data shadowing, which ensures that in a race condition, _both values are stored_
void UArtilleryDispatch::CheckFutures()
{
	
}

void UArtilleryDispatch::RERunGuns()
{
	if (ActionsToReconcile && ActionsToReconcile.IsValid())
	{
		throw;
	}
}

void UArtilleryDispatch::RERunLocomotions()
{
	throw;
}

void UArtilleryDispatch::LoadGunData()
{
#if UE_BUILD_SHIPPING != 0
		throw;
#endif
	FString AccumulatePath = FPaths::Combine(FPaths::ProjectPluginsDir(), "Artillery", "Data", "GunData");
		
}
//unused atm, but will be the way to ask for an eventish or triggered gun to fire, probably.
void UArtilleryDispatch::QueueFire(FGunKey Key, ArtilleryTime Time)
{
	if (ActionsToOrder && ActionsToOrder.IsValid())
	{
		ActionsToOrder->Enqueue(std::pair<FGunKey, ArtilleryTime>(Key, Time));
	}
}
