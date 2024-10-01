// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BarrageDispatch.h"
#include "SkeletonTypes.h"
#include "KeyCarry.h"
#include "FBarragePrimitive.h"
#include "Components/ActorComponent.h"
#include "BarrageGravityOnlyTester.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UBarrageGravityOnlyTester : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBarrageGravityOnlyTester();
	UBarrageGravityOnlyTester(const FObjectInitializer& ObjectInitializer);
	FBLet MyBarrageBody = nullptr;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FSkeletonKey MyObjectKey;
	bool IsReady = false;
	virtual void BeforeBeginPlay(FSkeletonKey TransformOwner);
	void Register();

	virtual void OnDestroyPhysicsState() override;
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};

//CONSTRUCTORS
//--------------------
//do not invoke the default constructor unless you have a really good plan. in general, let UE initialize your components.
inline UBarrageGravityOnlyTester::UBarrageGravityOnlyTester()
{
	PrimaryComponentTick.bCanEverTick = true;
	MyObjectKey = 0;
}
// Sets default values for this component's properties
inline UBarrageGravityOnlyTester::UBarrageGravityOnlyTester(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	
	PrimaryComponentTick.bCanEverTick = true;
	MyObjectKey = 0;
	
}
//---------------------------------

//SETTER: Unused example of how you might set up a registration for an arbitrary key.
inline void UBarrageGravityOnlyTester::BeforeBeginPlay(FSkeletonKey TransformOwner)
{
	MyObjectKey = TransformOwner;
}

//KEY REGISTER, initializer, and failover.
//----------------------------------

inline void UBarrageGravityOnlyTester::Register()
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
	if(!IsReady && MyObjectKey != 0) // this could easily be just the !=, but it's better to have the whole idiom in the example
	{
		auto Physics =  GetWorld()->GetSubsystem<UBarrageDispatch>();
		auto TransformECS =  GetWorld()->GetSubsystem<UTransformDispatch>();
		auto params = FBarrageBounder::GenerateBoxBounds(GetOwner()->GetActorLocation(), 30, 30 ,20);
		MyBarrageBody = Physics->CreatePrimitive(params, MyObjectKey, Layers::MOVING);
		//TransformECS->RegisterObjectToShadowTransform(MyObjectKey, const_cast<UE::Math::TTransform<double>*>(&GetOwner()->GetTransform()));
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


inline void UBarrageGravityOnlyTester::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Register();// ...
}

// Called when the game starts
inline void UBarrageGravityOnlyTester::BeginPlay()
{
	Super::BeginPlay();
	Register();
}

//TOMBSTONERS

inline void UBarrageGravityOnlyTester::OnDestroyPhysicsState()
{
	Super::OnDestroyPhysicsState();
	if(GetWorld())
	{
		auto Physics =  GetWorld()->GetSubsystem<UBarrageDispatch>();
		if(Physics && MyBarrageBody)
		{
			Physics->SuggestTombstone(MyBarrageBody);
			MyBarrageBody.Reset();
		}
	}
}

inline void UBarrageGravityOnlyTester::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if(GetWorld())
	{
		auto Physics =  GetWorld()->GetSubsystem<UBarrageDispatch>();
		if(Physics && MyBarrageBody)
		{
			Physics->SuggestTombstone(MyBarrageBody);
			MyBarrageBody.Reset();
		}
	}
}
