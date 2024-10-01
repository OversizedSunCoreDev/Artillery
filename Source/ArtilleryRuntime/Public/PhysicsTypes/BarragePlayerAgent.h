// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BarrageColliderBase.h"
#include "BarrageDispatch.h"
#include "SkeletonTypes.h"
#include "KeyCarry.h"
#include "FBarragePrimitive.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "BarragePlayerAgent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ARTILLERYRUNTIME_API UBarragePlayerAgent : public UBarrageColliderBase
{
	GENERATED_BODY()

	//This leans HARD on the collider base but retains more uniqueness than the others.
public:
	using Caps = 
	UE::Geometry::FCapsule3d;
	// Sets default values for this component's properties
	UPROPERTY()
	double radius;
	UPROPERTY()
	double extent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement, meta=(ClampMin="0", UIMin="0"))
	float TurningBoost = 1.1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float MaxVelocity = 600;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float Deceleration = 200;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float Acceleration = 200;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float AirAcceleration = 7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float JumpImpulse = 1000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement)
	float WallJumpImpulse = 500;



	[[nodiscard]] FVector Chaos_LastGameFrameRightVector() const
	{
		return CHAOS_LastGameFrameRightVector.IsNearlyZero() ? FVector::RightVector : CHAOS_LastGameFrameRightVector;
	}

	[[nodiscard]] FVector Chaos_LastGameFrameForwardVector() const
	{
		return CHAOS_LastGameFrameForwardVector.IsNearlyZero() ? FVector::ForwardVector : CHAOS_LastGameFrameForwardVector ;
	}

	UBarragePlayerAgent(const FObjectInitializer& ObjectInitializer);
	virtual void Register() override;
	void AddForce(float Duration);
	void ApplyRotation(float Duration, FQuat4f Rotation);
	void AddOneTickOfForce(FVector3d Force);
	// Kludge for now until we double-ify everything
	void AddOneTickOfForce(FVector3f Force);
	FVector3f GetVelocity();
	FVector3f GetGroundNormal();
	bool IsOnGround();
	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	UPROPERTY(BlueprintReadOnly)
	FVector CHAOS_LastGameFrameRightVector = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly)
	FVector CHAOS_LastGameFrameForwardVector = FVector::ZeroVector;
};

//CONSTRUCTORS
//--------------------
//do not invoke the default constructor unless you have a really good plan. in general, let UE initialize your components.

// Sets default values for this component's properties
inline UBarragePlayerAgent::UBarragePlayerAgent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	
	PrimaryComponentTick.bCanEverTick = true;
	MyObjectKey = 0;
	
}

inline FVector3f UBarragePlayerAgent::GetVelocity()
{
	return FBarragePrimitive::GetVelocity(MyBarrageBody);
}

inline FVector3f UBarragePlayerAgent::GetGroundNormal()
{
	return FBarragePrimitive::GetCharacterGroundNormal(MyBarrageBody);
}

inline bool UBarragePlayerAgent::IsOnGround()
{
	return FBarragePrimitive::IsCharacterOnGround(MyBarrageBody);
}
//KEY REGISTER, initializer, and failover.
//----------------------------------

inline void UBarragePlayerAgent::Register()
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
	if(!IsReady && MyObjectKey != 0 && !GetOwner()->GetActorLocation().ContainsNaN()) // this could easily be just the !=, but it's better to have the whole idiom in the example
	{
		auto Physics =  GetWorld()->GetSubsystem<UBarrageDispatch>();
			auto params = FBarrageBounder::GenerateCharacterBounds(GetOwner()->GetActorLocation(), radius, extent, MaxVelocity);
			MyBarrageBody = Physics->CreatePrimitive(params, MyObjectKey, Layers::MOVING);

			if(MyBarrageBody)
			{
				IsReady = true;
			}
	}
}

inline void UBarragePlayerAgent::AddForce(float Duration)
{
	//I'll be back for youuuu.
	throw;
}

inline void UBarragePlayerAgent::ApplyRotation(float Duration, FQuat4f Rotation)
{
	//I'll be back for youuuu.
	throw;
}

inline void UBarragePlayerAgent::AddOneTickOfForce(FVector3d Force)
{
	FBarragePrimitive::ApplyForce(Force, MyBarrageBody);
}

inline void UBarragePlayerAgent::AddOneTickOfForce(FVector3f Force)
{
	FBarragePrimitive::ApplyForce(FVector3d(Force.X, Force.Y, Force.Z), MyBarrageBody);
}

// Called when the game starts
inline void UBarragePlayerAgent::BeginPlay()
{
	Super::BeginPlay();
	Register();
}

inline void UBarragePlayerAgent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if(!IsReady)
	{
		Register();// ...
	}

	CHAOS_LastGameFrameRightVector = GetOwner()->GetActorRightVector();
	CHAOS_LastGameFrameForwardVector = GetOwner()->GetActorForwardVector();
	
}
