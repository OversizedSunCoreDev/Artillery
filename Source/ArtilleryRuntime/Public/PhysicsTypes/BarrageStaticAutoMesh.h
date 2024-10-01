// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BarrageColliderBase.h"
#include "BarrageDispatch.h"
#include "SkeletonTypes.h"
#include "KeyCarry.h"
#include "FBarragePrimitive.h"
#include "Components/ActorComponent.h"
#include "BarrageStaticAutoMesh.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARTILLERYRUNTIME_API UBarrageStaticAutoMesh : public UBarrageColliderBase
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBarrageStaticAutoMesh(const FObjectInitializer& ObjectInitializer);
	virtual void Register() override;


		
};
//CONSTRUCTORS
//--------------------

// Sets default values for this component's properties
inline UBarrageStaticAutoMesh::UBarrageStaticAutoMesh(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	MyObjectKey = 0;
	
}

inline void UBarrageStaticAutoMesh::Register()
{
	if(MyObjectKey ==0 )
	{
		if(GetOwner())
		{
			if(GetOwner()->GetComponentByClass<UKeyCarry>())
			{
				MyObjectKey = GetOwner()->GetComponentByClass<UKeyCarry>()->GetObjectKey();
			}
			
			if(MyObjectKey == 0)
			{
				auto val = PointerHash(GetOwner());
				ActorKey TopLevelActorKey = ActorKey(val);
				MyObjectKey = TopLevelActorKey;
			}
		}
	}
	if(!IsReady && MyObjectKey != 0 && GetOwner()) // this could easily be just the !=, but it's better to have the whole idiom in the example
	{
		auto Physics =  GetWorld()->GetSubsystem<UBarrageDispatch>();
		auto TransformECS =  GetWorld()->GetSubsystem<UTransformDispatch>();
		auto Actor = GetOwner();
		auto MeshPtr = Actor->GetComponentByClass<UStaticMeshComponent>();
		if(MeshPtr)
		{
			auto origin = Actor->GetActorLocation();
			origin = origin.GridSnap(1);
			FBMeshParams params = FBMeshParams(origin, 1);
			MyBarrageBody = Physics->LoadComplexStaticMesh(params, MeshPtr, MyObjectKey);
		}
		if(MyBarrageBody)
		{
			IsReady = true;
		}
	}
	if(IsReady)
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}