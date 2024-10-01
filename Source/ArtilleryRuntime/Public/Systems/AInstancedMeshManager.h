// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FWorldSimOwner.h"
#include "GameFramework/Actor.h"
#include "UEnemyMachine.h"
#include "SwarmKine.h"
#include "EPhysicsLayer.h"
#include "AInstancedMeshManager.generated.h"

UCLASS()
class ARTILLERYRUNTIME_API AInstancedMeshManager : public AActor
{
	GENERATED_BODY()

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Artillery, meta = (AllowPrivateAccess = "true"))
	USwarmKineManager* SwarmKineManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Artillery, meta = (AllowPrivateAccess = "true"))
	UArtilleryDispatch* MyDispatch;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Artillery, meta = (AllowPrivateAccess = "true"))
	UTransformDispatch* TransformDispatch;

	uint32 instances_generated;

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		if(!IsDefaultSubobject())
		{
			MyDispatch = GetWorld()->GetSubsystem<UArtilleryDispatch>();
			TransformDispatch = GetWorld()->GetSubsystem<UTransformDispatch>();
			// No Chaos for you!
			SwarmKineManager->SetEnableGravity(false);
			SwarmKineManager->SetSimulatePhysics(false);
			SwarmKineManager->DestroyPhysicsState();

			// Make a key yo
			auto keyHash = PointerHash(this);
			UE_LOG(LogTemp, Warning, TEXT("AInstancedMeshManager Parented: %d"), keyHash);
			MyKey = ActorKey(keyHash);
			Usable = true;
		}
	}

public:
	bool Usable = false;
	
	AInstancedMeshManager()
	{
		SwarmKineManager = CreateDefaultSubobject<USwarmKineManager>("SwarmKineManager");
		SwarmKineManager->bDisableCollision = true;
		MyDispatch = nullptr;
		TransformDispatch = nullptr;
	}
	
	ActorKey GetMyKey() const
	{
		return MyKey;
	};

	void SetStaticMesh(UStaticMesh* Mesh)
	{
		SwarmKineManager->SetStaticMesh(Mesh);
	}

	UFUNCTION(BlueprintCallable, Category = Instance)
	FSkeletonKey CreateNewInstance(const FTransform& WorldTransform, const FVector& MuzzleVelocity, const EPhysicsLayer Layer, bool IsSensor = false)
	{
		return CreateNewInstance(WorldTransform, MuzzleVelocity, static_cast<uint16_t>(Layer), IsSensor);
	}
	FSkeletonKey CreateNewInstance(const FTransform& WorldTransform, const FVector3d& MuzzleVelocity, const uint16_t Layer, bool IsSensor = false)
	{
		FPrimitiveInstanceId NewInstanceId = SwarmKineManager->AddInstanceById(WorldTransform, true);
		// TODO: Does this make a good hash? Can we hash collide?
		// TODO: Oh god this definitely birthday problems at some point but I don't know how else to get a unique hash since the instances rotate around and reuse the same memory
		auto hash = PointerHash(SwarmKineManager, ++instances_generated);
		UE_LOG(LogTemp, Warning, TEXT("NewInstanceId: %i"), hash);
		FSkeletonKey NewInstanceKey = FSkeletonKey(hash);

		SwarmKineManager->AddToMap(NewInstanceId, NewInstanceKey);

		// TODO: can't use the BarrageColliderBase set of types, so in-lining the barrage setup code. Is this what we want long-term?
		auto Physics = GetWorld()->GetSubsystem<UBarrageDispatch>();
		auto TransformECS =  GetWorld()->GetSubsystem<UTransformDispatch>();
		auto AnyMesh = SwarmKineManager->GetStaticMesh();
		auto Boxen = AnyMesh->GetBoundingBox();
		auto extents = Boxen.GetExtent() * 2;

		auto params = FBarrageBounder::GenerateBoxBounds(WorldTransform.GetLocation(), extents.X, extents.Y, extents.Z,
			FVector3d(0, 0, extents.Z/2));

		FBLet MyBarrageBody = Physics->CreatePrimitive(params, NewInstanceKey, Layer, IsSensor);
		FBarragePrimitive::SetVelocity(MuzzleVelocity, MyBarrageBody);

		TransformDispatch->RegisterObjectToShadowTransform(NewInstanceKey, SwarmKineManager);

		MyDispatch->REGISTER_PROJECTILE_FINAL_TICK_RESOLVER(100, NewInstanceKey);
		
		return NewInstanceKey;
	}

	// THIS MUST BE CALLED OR ELSE THE MAPPINGS WILL KEEP THE LIVE REFERENCE 4EVA

	void CleanupInstance(const FSkeletonKey Target)
	{
		// TODO: Not sure how if this cleans up the FBLet in Jolt
		// Tombstones don't seem to do anything? UBarrageDispatch::Entomb is never called
		auto Physics = GetWorld()->GetSubsystem<UBarrageDispatch>();
		Physics->SuggestTombstone(Physics->GetShapeRef(Target));
		SwarmKineManager->CleanupInstance(Target);
		TransformDispatch->ReleaseKineByKey(Target);
	}

private:
	ActorKey MyKey;
};