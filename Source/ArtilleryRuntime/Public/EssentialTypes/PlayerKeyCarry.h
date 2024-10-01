// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkeletonTypes.h"
#include "TransformDispatch.h"
#include "KeyCarry.h"
#include "PlayerKine.h"
#include "Runtime/Engine/Classes/Components/ActorComponent.h"
#include "PlayerKeyCarry.generated.h"

//this is a simple key-carrier that automatically wires the player up.
//It tries during initialize, and if that fails, it tries again each tick until successful,
//then turns off ticking for itself. Clients should use the Retry_Notify delegate to register
//for notification of success in production code, rather than relying on initialization sequencing.
//Later versions will also set a gameplay tag to indicate that this actor carries a key.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), DefaultToInstanced)
class ARTILLERYRUNTIME_API UPlayerKeyCarry : public UKeyCarry
{
	GENERATED_BODY()
	FSkeletonKey MyObjectKey;
public:
	DECLARE_MULTICAST_DELEGATE(ActorKeyIsReady)
	ActorKeyIsReady Retry_Notify;
	bool isReady = false;
	
	UPlayerKeyCarry(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
	{
		// While we init, we do not tick more than once.. This is a "data only" component. gotta be a better way to do this.
		PrimaryComponentTick.bCanEverTick = true;
		bWantsInitializeComponent = true;
		// ...
	}
	

	virtual void AttemptRegister() override
	{
		if(GetWorld())
		{
			if(auto xRef = GetWorld()->GetSubsystem<UTransformDispatch>())
			{
				if(TObjectPtr<AActor> actorRef = GetOwner())
				{
					
					actorRef->UpdateComponentTransforms();
					if(actorRef)
					{
						//I think this will end up diverging more than usual.
						auto val = PointerHash(GetOwner());
						ActorKey TopLevelActorKey = ActorKey(val);
						MyObjectKey = TopLevelActorKey;
						xRef->RegisterObjectToShadowTransform<PlayerKine, ActorKey, TObjectPtr<AActor> >(MyObjectKey ,actorRef);
						isReady = true;
						if(Retry_Notify.IsBound())
						{
							Retry_Notify.Broadcast();
						}
						SetComponentTickEnabled(false);
					}
				}
			}
		}
	}
	
	//will return an invalid object key if it fails.
	static inline FSkeletonKey KeyOfPlayer(AActor* That)
	{
	if(That)
	{
		if(That->GetComponentByClass<UPlayerKeyCarry>())
		{
			return That->GetComponentByClass<UPlayerKeyCarry>()->MyObjectKey;
		}
	}
	return FSkeletonKey();
	}
	
};
