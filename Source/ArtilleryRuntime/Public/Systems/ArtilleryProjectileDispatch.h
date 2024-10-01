// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AInstancedMeshManager.h"
#include "ArtilleryCommonTypes.h"
#include "ArtilleryDispatch.h"
#include "FProjectileDefinitionRow.h"
#include "ArtilleryProjectileDispatch.generated.h"

/**
 * This is the Artillery subsystem that manages the lifecycle of projectiles using only SkeletonKeys rather than UE5
 * actors. This is done by instancing projectiles through AInstancedMeshManager actors rather than creating an actor
 * per projectile which greatly reduces computational load as all projectiles with the same model can be managed by
 * Artillery + TickLites rather than through the heavy and expensive UE5 Actor system.
 *
 * This does mean that a lot of UE5 default functionality associated with Actors won't and don't work with Artillery
 * Projectiles, but this is intended. Artillery Projectiles should be managed through this subsystem and through the
 * ticklites that are assigned to them (also through this subsystem). This subsystem is responsible for basic default
 * behavior of projectiles that Artillery supports, but additional behavior can be added to Artillery Projectiles by
 * way of attaching custom TickLites to them.
 *
 */

namespace Arty
{
	DECLARE_MULTICAST_DELEGATE(OnArtilleryProjectilesActivated);
}

UCLASS()
class ARTILLERYRUNTIME_API UArtilleryProjectileDispatch : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	static inline UArtilleryProjectileDispatch* SelfPtr = nullptr;
public:
	OnArtilleryProjectilesActivated BindToArtilleryProjectilesActivated;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UDataTable* ProjectileDefinitions;
	TSharedPtr<TMap<FSkeletonKey, TWeakObjectPtr<AInstancedMeshManager>>> ManagerKeyToMeshManagerMapping;
	TSharedPtr<TMap<FSkeletonKey, TWeakObjectPtr<AInstancedMeshManager>>> ProjectileKeyToMeshManagerMapping;
	TSharedPtr<TMap<FName, TWeakObjectPtr<AInstancedMeshManager>>> ProjectileNameToMeshManagerMapping;

public:
	virtual void PostInitialize() override;

	FProjectileDefinitionRow* GetProjectileDefinitionRow(const FName ProjectileDefinitionId);
	FSkeletonKey CreateProjectileInstance(const FName ProjectileDefinitionId, const FTransform& WorldTransform, const FVector3d& MuzzleVelocity, const bool IsSensor);
	void DeleteProjectile(const FSkeletonKey Target);
	TWeakObjectPtr<AInstancedMeshManager> GetProjectileMeshManagerByManagerKey(const FSkeletonKey ManagerKey);
	TWeakObjectPtr<AInstancedMeshManager> GetProjectileMeshManagerByProjectileKey(const FSkeletonKey ProjectileKey);

	void OnBarrageContactAdded(const BarrageContactEvent& ContactEvent);
	
};
