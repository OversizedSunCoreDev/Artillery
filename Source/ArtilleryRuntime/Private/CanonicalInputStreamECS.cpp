// Fill out your copyright notice in the Description page of Project Settings.


#include "CanonicalInputStreamECS.h"

void UCanonicalInputStreamECS::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("Artillery::CanonicalInputStream is Online."));
}

void UCanonicalInputStreamECS::OnWorldBeginPlay(UWorld& InWorld)
{
	if ([[maybe_unused]] const UWorld* World = InWorld.GetWorld()) {
		UE_LOG(LogTemp, Warning, TEXT("Artillery::CanonicalInputStream is Operational"));
		MySquire = GetWorld()->GetSubsystem<UBristleconeWorldSubsystem>();
		}
	SelfPtr = this;
}

void UCanonicalInputStreamECS::Deinitialize()
{
	SelfPtr = NULL;
	UE_LOG(LogTemp, Warning, TEXT("Artillery::CanonicalInputStream is Shutting Down."));
	Super::Deinitialize();
}

ActorKey UCanonicalInputStreamECS::ActorByStream(InputStreamKey Stream)
{
	return StreamToActorMapping->FindRef(Stream);
}

InputStreamKey UCanonicalInputStreamECS::StreamByActor(ActorKey Actor)
{
	return ActorToStreamMapping->FindRef(Actor);
}

void UCanonicalInputStreamECS::Tick(float DeltaTime)
{
}

TStatId UCanonicalInputStreamECS::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCanonicalInputStreamECS, STATGROUP_Tickables);
}


//OH NO. HIDDEN ORDERING DEPENDENCY EMERGED! if this gets called AFTER getstreamforplayer, which can happen because we now handle
//the CDO correctly, we can end up in a situation where things just sort of flywheel off to hell because there's not actually any stream
//constructs extant yet.
TSharedPtr<UCanonicalInputStreamECS::FConservedInputStream> UCanonicalInputStreamECS::getNewStreamConstruct( PlayerKey ByPlayerConcept)
{
	
	TSharedPtr<ArtilleryControlStream> ManagedStream = MakeShareable(
	new FConservedInputStream(this, ByPlayerConcept) //using++ vs ++would be wrong here. inc then ret.
	);
	auto BifurcateOwnership = new TSharedPtr<ArtilleryControlStream>(ManagedStream);
	//fun fucking story, this was working by ACCIDENT because we were somehow ZEROING OUT the pointers, causing things to JUST BARELY map.
	//here we go again.
	SessionPlayerToStreamMapping->Add(ByPlayerConcept, ManagedStream->MyKey);//
	StreamKeyToStreamMapping->Add(ManagedStream->MyKey, *BifurcateOwnership);//This is the key driver for the ordering problem
	return ManagedStream; 
}


InputStreamKey UCanonicalInputStreamECS::GetStreamForPlayer(PlayerKey ThisPlayer)
{
	//TODO: this can actually fail if the start up sequence happens in a really unusual order.
	return SessionPlayerToStreamMapping->FindChecked(ThisPlayer);
}

TSharedPtr<UCanonicalInputStreamECS::FConservedInputStream> UCanonicalInputStreamECS::GetStream(InputStreamKey StreamKey) const
{
	const auto SP = StreamKeyToStreamMapping->FindRef(StreamKey);
	return SP; // creates a copy.
}

bool UCanonicalInputStreamECS::registerPattern( IPM::CanonPattern ToBind,
                                               FActionPatternParams FCM_Owner_ActorParams)
{
	if (
#ifndef LOCALISCODEDSPECIAL
		FCM_Owner_ActorParams.MyInputStream == APlayer::CABLE ||
#endif // !LOCALISCODEDSPECIAL
		StreamKeyToStreamMapping->Contains(FCM_Owner_ActorParams.MyInputStream))
	{
		auto thisInputStream = StreamKeyToStreamMapping->Find(FCM_Owner_ActorParams.MyInputStream)->Get();
		if (thisInputStream->MyPatternMatcher->AllPatternBinds.Contains(ToBind->getName()))
		{
			//names are never removed. sets are only added to or removed from.
			thisInputStream->MyPatternMatcher->AllPatternBinds.Find(ToBind->getName())->Get()->Add(FCM_Owner_ActorParams);

		}
		else
		{
			thisInputStream->MyPatternMatcher->Names.Add(ToBind->getName());
			thisInputStream->MyPatternMatcher->AllPatternsByName.Add(ToBind->getName(), ToBind);
			TSharedPtr<TSet<FActionPatternParams>> newSet = MakeShareable < TSet<FActionPatternParams>>(new TSet<FActionPatternParams>());
			newSet.Get()->Add(FCM_Owner_ActorParams);
			thisInputStream->MyPatternMatcher->AllPatternBinds.Add(ToBind->getName(), newSet);
		}

		return true;
	}
	return false;
}

bool UCanonicalInputStreamECS::removePattern(IPM::CanonPattern ToBind, FActionPatternParams FCM_Owner_ActorParams)
{
	if (
#ifndef LOCALISCODEDSPECIAL
		FCM_Owner_ActorParams.MyInputStream == APlayer::CABLE ||
#endif // !LOCALISCODEDSPECIAL
		StreamKeyToStreamMapping->Contains(FCM_Owner_ActorParams.MyInputStream))
	{
		auto thisInputStream = StreamKeyToStreamMapping->Find(FCM_Owner_ActorParams.MyInputStream)->Get();
		if (thisInputStream->MyPatternMatcher->AllPatternBinds.Contains(ToBind->getName()))
		{
			//names are never removed. sets are only added to or removed from.
			auto pinSharedPtr = thisInputStream->MyPatternMatcher->AllPatternBinds.Find(ToBind->getName());

			if (pinSharedPtr->Get()->Contains(FCM_Owner_ActorParams))
			{
				auto remId = pinSharedPtr->Get()->FindId(FCM_Owner_ActorParams);
				pinSharedPtr->Get()->Remove(remId);
				return true;
			}
		}
		
	}
	return false;
}
TPair<ActorKey, InputStreamKey> UCanonicalInputStreamECS::RegisterKeysToParentActorMapping(AActor* parent, FireControlKey MachineKey, bool IsActorForLocalPlayer)
{
	//todo, registration goes here.
	auto val = PointerHash(parent);
	UE_LOG(LogTemp, Warning, TEXT("FCM Parented: %d"), val);
	ActorKey ParentKey(val);
	LocalActorToFireControlMapping->Add(ParentKey, MachineKey);

	//this is a hack. this is such a hack. oh god.
	if(IsActorForLocalPlayer)
	{
#if UE_BUILD_SHIPPING != 0
		throw; //I told you not to ship this without checking.
#endif
		//this relies on a really ugly hack using the ENUM. do not ship this without being sure you want to.
		InputStreamKey LocalKey = GetStreamForPlayer(APlayer::CABLE);
		StreamToActorMapping->Add(LocalKey, ParentKey); //ONE OF THE TWO THINGS IS WRONG NOW, CONGRATS, HERO.
		ActorToStreamMapping->Add(ParentKey, LocalKey);
		return TPair<ActorKey, InputStreamKey>(ParentKey, LocalKey);			
	}
	else
	{
		throw;
	}

}


