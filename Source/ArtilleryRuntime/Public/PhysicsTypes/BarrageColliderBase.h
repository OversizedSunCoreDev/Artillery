// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BarrageDispatch.h"
#include "SkeletonTypes.h"
#include "KeyCarry.h"
#include "FBarragePrimitive.h"
#include "Components/ActorComponent.h"
#include "BarrageColliderBase.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UBarrageColliderBase : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBarrageColliderBase(const FObjectInitializer& ObjectInitializer);
	virtual void InitializeComponent() override;
	FBLet MyBarrageBody = nullptr;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FSkeletonKey MyObjectKey;
	bool IsReady = false;
	virtual void BeforeBeginPlay(FSkeletonKey TransformOwner);

	//Colliders must override this.
	virtual void Register();

	virtual void OnDestroyPhysicsState() override;
	// Called when the game starts
	virtual void BeginPlay() override;

	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};

//CONSTRUCTORS
//--------------------
//do not invoke the default constructor unless you have a really good plan. in general, let UE initialize your components.

// Sets default values for this component's properties
inline UBarrageColliderBase::UBarrageColliderBase(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	
	PrimaryComponentTick.bCanEverTick = true;
	
	bWantsInitializeComponent = true;
	MyObjectKey = 0;
	
}
//---------------------------------


inline void UBarrageColliderBase::InitializeComponent() 
{
	Super::InitializeComponent();
}

//SETTER: Unused example of how you might set up a registration for an arbitrary key.
inline void UBarrageColliderBase::BeforeBeginPlay(FSkeletonKey TransformOwner)
{
	MyObjectKey = TransformOwner;
}

//Colliders must override this.
inline  void UBarrageColliderBase::Register()
{
	PrimaryComponentTick.SetTickFunctionEnable(false);
}


inline void UBarrageColliderBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Register();// ...
}

// Called when the game starts
inline void UBarrageColliderBase::BeginPlay()
{
	Super::BeginPlay();
	Register();
}

//TOMBSTONERS

inline void UBarrageColliderBase::OnDestroyPhysicsState()
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

inline void UBarrageColliderBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
