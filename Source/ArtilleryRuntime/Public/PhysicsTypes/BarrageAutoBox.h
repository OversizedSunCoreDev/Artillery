// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BarrageColliderBase.h"
#include "BarrageDispatch.h"
#include "SkeletonTypes.h"
#include "KeyCarry.h"
#include "FWorldSimOwner.h"
#include "Components/ActorComponent.h"
#include "BarrageAutoBox.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), DefaultToInstanced )
class ARTILLERYRUNTIME_API UBarrageAutoBox : public UBarrageColliderBase
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool isMovable = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OffsetCenterToMatchBoundedShapeX = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OffsetCenterToMatchBoundedShapeY = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OffsetCenterToMatchBoundedShapeZ = 0;
	UBarrageAutoBox(const FObjectInitializer& ObjectInitializer);
	virtual void Register() override;
};

//CONSTRUCTORS
//--------------------
//do not invoke the default constructor unless you have a really good plan. in general, let UE initialize your components.

// Sets default values for this component's properties
inline UBarrageAutoBox::UBarrageAutoBox(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	
	bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	MyObjectKey = 0;
	
}

//KEY REGISTER, initializer, and failover.
//----------------------------------

inline void UBarrageAutoBox::Register()
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
		auto AnyMesh = GetOwner()->GetComponentByClass<UPrimitiveComponent>();
		auto Boxen = AnyMesh->GetLocalBounds();
		// Multiply by the scale factor, then multiply by 2 since mesh bounds is radius not diameter
		auto extents = Boxen.BoxExtent * AnyMesh->BoundsScale * 2; 
		
		auto params = FBarrageBounder::GenerateBoxBounds(GetOwner()->GetActorLocation(),extents.X, extents.Y, extents.Z,
			FVector3d(OffsetCenterToMatchBoundedShapeX, OffsetCenterToMatchBoundedShapeY, extents.Z/2));
		MyBarrageBody = Physics->CreatePrimitive(params, MyObjectKey,  isMovable ? Layers::MOVING : Layers::NON_MOVING);
		//TransformECS->RegisterObjectToShadowTransform(MyObjectKey, const_cast<UE::Math::TTransform<double>*>(&GetOwner()->GetTransform()));
		if(MyBarrageBody)
		{
			AnyMesh->WakeRigidBody();
			IsReady = true;
			AnyMesh->SetSimulatePhysics(false);
		}
	}
	if(IsReady)
	{
		PrimaryComponentTick.SetTickFunctionEnable(false);
	}
}