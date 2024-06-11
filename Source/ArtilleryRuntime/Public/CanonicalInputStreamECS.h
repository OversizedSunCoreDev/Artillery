// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "Templates/SubclassOf.h"
#include "UObject/UnrealType.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
#include "Containers/CircularBuffer.h"
#include "BristleconeCommonTypes.h"
#include "UBristleconeWorldSubsystem.h"
#include <optional>
#include <unordered_map>
#include <ArtilleryShell.h>
#include "ArtilleryCommonTypes.h"
#include "CanonicalInputStreamECS.generated.h"





//TODO: finish adding the input streams, replace the local handling in Bristle54 character with references to the input stream ecs
//TODO: begin work on the conceptual frame for reconciling and assessing what input does and does not exist.
//AXIOMS: Input we send is real. Input we get in the batches represents the full knowledge of the server.
//AXIOM: A missed batch means we could be starting to desynchronize.
//AXIOM: Bristlecone Time + the fixed cadencing allows us to know when something SHOULD arrive.
//FACT: A batch missing input of OURS that we believe should be there could mean we are starting to desynchronize.
//FACT: A batch containing input of OURS that we believe should have been in an EARLIER batch could mean we are starting to desynchronize.
//FACT: The server will always have less of our input than we do.
//FACT: We may see input from other players before the server processes it.
//FACT: However, if it's missing from a batch, the server hasn't seen it yet.
//FACT: We may get server update pushes older than our input and/or older than the newest batch we have.
//FACT: We may have different orderings.
//FACT: we can determine the correct ordering and we all know the correct ordering of all input that made it into batches.

//Notes:
/*
Multiversus just relays input, it's all deterministic, and it records it.
At the end of the game, if clients disagree, it spins up a simulation and replays the inputs,
and uses that for the outcome (and presumably flags whoever disagreed for statistical detection).
*/


UCLASS()
class ARTILLERYRUNTIME_API UCanonicalInputStreamECS : public UTickableWorldSubsystem
{
	GENERATED_BODY()

	/**
	 * Conserved input streams record their last 8,192 inputs. Yes, that's a few megs across all streams. No, it doesn't really seem to matter.
	 * Currently, this is for debug purposes, but we can use it with some additional features to provide a really expressive
	 * model for rollback at a SUPER granular level if needed. UE has existing rollback tech, though so...
	 *
	 * These streams are designed for single reader, single producer, but it is hypothetically possible to have a format where you do
	 * single producer, single consumer, multiple observer. I don't recommend that, due to the need to mark records as played for cosmetics.
	 *
	 * It is possible that an observer might end up with a stale view of if a record's cosmetic effects have been applied, in this circumstance.
	 * This is DONE DURING THE GET. That can lead to an unholy mess. If you need an observer, ensure that it does not regard cosmetics as important
	 * AND use a PEEK.
	 */
public:
	ArtilleryTime Now()
	{
		return MySquire->Now();
	};
	static const uint32_t InputConservationWindow = 8192;
	static const uint32_t AddressableInputConservationWindow = InputConservationWindow - (2 * TheCone::LongboySendHertz);
	friend class FArtilleryBusyWorker;
	friend class UArtilleryDispatch;

	bool registerPattern(TSharedPtr<FActionPattern> ToBind, FActionBitMask ToSeek, FGunKey ToFire, ActorKey FCM_Owner_Actor);
	bool removePattern(TSharedPtr<FActionPattern> ToBind, FActionBitMask ToSeek, FGunKey ToFire, ActorKey FCM_Owner_Actor);
	ActorKey registerFCMKeyToParentActorMapping(AActor* parent, FireControlKey MyKey);
	class ARTILLERYRUNTIME_API FConservedInputStream
	{
		friend class FArtilleryBusyWorker;
	public:
		TCircularBuffer<FArtilleryShell> CurrentHistory = TCircularBuffer<FArtilleryShell>(InputConservationWindow); //these two should be one buffer of a shared type, but that makes using them harder
		InputStreamKey MyKey; //in case, god help us, we need a lookup based on this for something else. that should NOT happen.


		//Correct usage procedure is to null check then store a copy.
		//Failure to follow this procedure will lead to eventual misery.
		//This has a side-effect of marking the record as played at least once.
		std::optional<FArtilleryShell> get(uint64_t input)
		{
			// the highest input is a reserved write-slot.
			//the lower bound here ensures that there's always minimum two seconds worth of memory separating the readers
			//and the writers. How safe is this? It's not! But it's insanely fast. Enjoy, future jake!
			//TODO: Refactor this to use an atomic int instead of this hubristic madness.
			if (input >= highestInput || (highestInput - input) > AddressableInputConservationWindow)
			{
				return std::optional<FArtilleryShell>(
					std::nullopt
				);
			}
			else {
				CurrentHistory[input].RunAtLeastOnce = true; //this is the only risky op in here from a threading perspective.
				return std::optional<FArtilleryShell>(CurrentHistory[input]);
			}
		};

		//THE ONLY DIFFERENCE WITH PEEK IS THAT IT DOES NOT SET RUNATLEASTONCE.
		//PEEK IS PROVIDED ONLY AS AN EMERGENCY OPTION, AND IF YOU FIND YOURSELF USING IT
		//MAY GOD HAVE MERCY ON YOUR SOUL.
		std::optional<FArtilleryShell> peek(uint64_t input)
		{
			// the highest input is a reserved write-slot.
			//the lower bound here ensures that there's always minimum two seconds worth of memory separating the readers
			//and the writers. How safe is this? It's not! But it's insanely fast. Enjoy, future jake!
			//TODO: Refactor this to use an atomic int instead of this hubristic madness.
			if (input >= highestInput || (highestInput - input) > AddressableInputConservationWindow)
			{
				return std::optional<FArtilleryShell>(
					std::nullopt
				);
			}
			else {
				return std::optional<FArtilleryShell>(CurrentHistory[input]);
			}
		};

		InputStreamKey GetInputStreamKeyByPlayer(PlayerKey SessionLevelPlayerID)
		{
			return 0;
		};

		InputStreamKey GetInputStreamKeyByLocalActorKey(ActorKey LocalLevelActorID)
		{
			return 0;
		};

	protected:
		volatile uint64_t highestInput = 0; // volatile is utterly useless for its intended purpose. 
		UCanonicalInputStreamECS* ECSParent;

		//Add can only be used by the Artillery Worker Thread through the methods of the UCISArty.
		void add(INNNNCOMING shell, long SentAt)
		{
			CurrentHistory[highestInput].MyInputActions = shell;
			CurrentHistory[highestInput].ReachedArtilleryAt = ECSParent->Now();
			CurrentHistory[highestInput].SentAt = SentAt;//this is gonna get weird after a couple refactors, but that's why we hide it here.

			// reading, adding one, and storing are all separate ops. a slice here is never dangerous but can be erroneous.
			// because this is a volatile variable, it cannot be optimized away and most compilers will not reorder it.
			// however, volatile is basically useless normally. it doesn't provoke a memory fence, and for a variety of reasons
			// it's not suitable for most driver applications. However.
			// 
			// There's a special case which is a monotonically increasing value that is only ever
			// incremented by one thread with a single call site for the increment. In this case, you can still get
			// interleaved but the value will always be either k or k+1. If it's stale in cache, the worst case
			// is that the newest input won't be legible yet and this can be resolved by repolling.
			++highestInput;
		};

		//Overload for local add via feed from cabling. don't use this unless you are CERTAIN.
		void add(INNNNCOMING shell)
		{
			CurrentHistory[highestInput].MyInputActions = shell;
			CurrentHistory[highestInput].ReachedArtilleryAt = ECSParent->Now();
			CurrentHistory[highestInput].SentAt = ECSParent->Now();
			++highestInput;
		};
	};

	//Used in the busyworker
	//There should only be one of these fuckers.
	//This is really just a way of grouping some of the functionality
	//of the overarching world subsystem together into an FClass that can
	//be used safely off thread without any consideration. you can use Uclasses if you're careful but...
	class ARTILLERYRUNTIME_API FConservedInputPatternMatcher
	{

	friend class FArtilleryBusyWorker;
	public:
		TCircularBuffer<FArtilleryShell> CurrentHistory = TCircularBuffer<FArtilleryShell>(ArtilleryInputSearchWindow);
		TSet<InputStreamKey> MyStreamKeys; //in case, god help us, we need a lookup based on this for something else. that should NOT happen.

		//there's a bunch of reasons we use string_view here, but mostly, it's because we can make them constexprs!
		//so this is... uh... pretty fast!
		TMap<FString, TSharedPtr<TSet<FActionPatternParams>>> AllPatternBinds; //broadly, at the moment, there is ONE pattern matcher running


		//this array is never made smaller.
		//there should only ever be about 10 patterns max,
		//and it's literally more expensive to remove them.
		//As a result, we track what's actually live via the binds
		//and this array is just lazy accumulative. it means we don't ever allocate a key array for TMap.
		//has some other advantages, as well.
		TArray<FString> Names;

		//same with this set, actually. patterns are stateless, and few. it's inefficient to destroy them.
		//instead we check binds.
		TMap<FString, TSharedPtr<FActionPattern_InternallyStateless>> AllPatternsByName;

		void GlassCurrentHistory()
		{
			CurrentHistory = TCircularBuffer<FArtilleryShell>(ArtilleryInputSearchWindow); //expect the search window to be big.
		};

		//***********************************************************
		//
		// THIS IS THE IMPORTANT FUNCTION.
		//
		// ***********************************************************
		//
		// This makes things run. currently, it doesn't correctly handle really anything
		// but it's come together now so that you can see what's happening.
		// This has a side-effect of marking the record as played at least once.
		//This will match patterns and push events up to the Fire Control Machines. There likely will only be 12 or 18 FCMs running
		//EVER because I think we'll want to treat each AI faction pretty much as a single FCM except for a few bosses.
		//hard to say. we might need to revisit this if the FCMs prove too heavy as full actor components.

		bool runOneFrameWithSideEffects(bool isResim_Unimplemented,
			//USED TO DEFINE HOW TO HIDE LATENCY BY TRIMMING LEAD-IN FRAMES OF AN ARTILLERYGUN
			uint32_t leftTrimFrames,
			//USED TO DEFINE HOW TO SHORTEN ARTILLERYGUNS BY SHORTENING TRAILING or INFIX DELAYS, SUCH AS DELAYED EXPLOSIONS, TRAJECTORIES, OR SPAWNS, TO HIDE LATENCY.
			uint32_t rightTrimFrames
		)
		{

			if (!isResim_Unimplemented)
			{
				UE_LOG(LogTemp, Display, TEXT("Still no resim, actually."));
			}

			for (FString Name : Names)
			{
				//note this checks BINDS. This is because our other mappings are _pure additive._
				//this isn't a vanity thing. this idiosyncracy makes actual thread synchro much much easier.
				//it's also why we aren't checking to see if the pointers are valid. if they _aren't_ then it's a lifecycle bug.
				if (AllPatternBinds.Contains(Name))
				{
					TSharedPtr<TSet<FActionPatternParams>> AllParamsForPattern = AllPatternBinds[Name];
					if (AllParamsForPattern->Num() < 0)
					{
						TSharedPtr<FActionPattern_InternallyStateless> currentPattern = AllPatternsByName[Name];
						for (auto& Elem : *AllParamsForPattern)
						{
							//todo: add up all the masks for the pattern to produce the union of candidate codings.
							//the runPattern should actually return a mask of the union of all
							//candidate codings that matched fully. this will be necessary to avoid an N^3 op.
							//we'll likely need EVERY artillery gun to have a default behavior for priority re: binds.
							//if I didn't absolutely need this "true threaded" to be able to do reconciles fast with jolt,
							//we would have used enhanced input. :/
							if (currentPattern->runPattern(Elem))
							{
								//NOW the logic HERE in the MATCHER should handle which binds actually fire, using the bind params.
								//actually, we might need to aggregate everything from the union matches THEN determine what happens
								//because we might have a pattern's input get eaten.
							}
						}
					}
				}
			}

			//we also need to do the movement control either here, or back in the busy worker separately.
			//and that means we need an answer for how to do stick-flicks. they're such an unusual input
			//that I'm really inclined to just hand-jam them as a sort of Weird Pattern you can subscribe to
			//that secretly checks the sticks.
			return true; //well take a nap ZEN fire ZE missiles.
		};
	protected:

	};

protected:
	//this will need to be triple buffered or synchroed soon. :/
	TSharedPtr<UCanonicalInputStreamECS::FConservedInputPatternMatcher> SingletonPatternMatcher;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
private:
	std::unordered_map <InputStreamKey, TSharedPtr<FConservedInputStream>>* InternalMapping;
	std::unordered_map <PlayerKey, InputStreamKey>* SessionPlayerToStreamMapping;
	std::unordered_map <ActorKey, InputStreamKey>* LocalActorToStreamMapping;
	UBristleconeWorldSubsystem* MySquire;

};


typedef UCanonicalInputStreamECS UCISArty;
typedef UCISArty::FConservedInputStream ArtilleryControlStream;
typedef ArtilleryControlStream FAControlStream;