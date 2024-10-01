// Fill out your copyright notice in the Description page of Project Settings.

#include "ArtilleryProjectileDispatch.h"
#include "BarrageDispatch.h"

void UArtilleryProjectileDispatch::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// TODO: Can we find and autoload the datatable, or do 
	ProjectileDefinitions = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, TEXT("DataTable'/Game/DataTables/ProjectileDefinitions.ProjectileDefinitions'")));
	ManagerKeyToMeshManagerMapping = MakeShareable(new TMap<FSkeletonKey, TWeakObjectPtr<AInstancedMeshManager>>());
	ProjectileKeyToMeshManagerMapping = MakeShareable(new TMap<FSkeletonKey, TWeakObjectPtr<AInstancedMeshManager>>());
	ProjectileNameToMeshManagerMapping = MakeShareable(new TMap<FName, TWeakObjectPtr<AInstancedMeshManager>>());
	SelfPtr = this;
	UE_LOG(LogTemp, Warning, TEXT("ArtilleryProjectileDispatch:Subsystem: Online"));
}

void UArtilleryProjectileDispatch::PostInitialize()
{
	Super::PostInitialize();
}

void UArtilleryProjectileDispatch::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UBarrageDispatch* BarrageDispatch = GetWorld()->GetSubsystem<UBarrageDispatch>();
	BarrageDispatch->OnBarrageContactAddedDelegate.AddUObject(this, &UArtilleryProjectileDispatch::OnBarrageContactAdded);
}

void UArtilleryProjectileDispatch::Deinitialize()
{
	Super::Deinitialize();
	ManagerKeyToMeshManagerMapping->Empty();
	ProjectileKeyToMeshManagerMapping->Empty();
	ProjectileNameToMeshManagerMapping->Empty();
}

FProjectileDefinitionRow* UArtilleryProjectileDispatch::GetProjectileDefinitionRow(const FName ProjectileDefinitionId)
{
	if (ProjectileDefinitions != nullptr)
	{
		FProjectileDefinitionRow* FoundRow = ProjectileDefinitions->FindRow<FProjectileDefinitionRow>(ProjectileDefinitionId, TEXT("ProjectileTableLibrary"));
		return FoundRow;
	}
	return nullptr;
}

FSkeletonKey UArtilleryProjectileDispatch::CreateProjectileInstance(const FName ProjectileDefinitionId, const FTransform& WorldTransform, const FVector3d& MuzzleVelocity, const bool IsSensor)
{
	auto MeshManagerPtr = ProjectileNameToMeshManagerMapping->Find(ProjectileDefinitionId);

	if (!MeshManagerPtr || !MeshManagerPtr->IsValid())
	{
		if (const FProjectileDefinitionRow* ProjectileDefinition = GetProjectileDefinitionRow(ProjectileDefinitionId))
		{
			if (UStaticMesh* StaticMeshPtr = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *ProjectileDefinition->ProjectileMeshLocation)))
			{
				AInstancedMeshManager* NewMeshManager = GetWorld()->SpawnActor<AInstancedMeshManager>();
				NewMeshManager->SetStaticMesh(StaticMeshPtr);
				ManagerKeyToMeshManagerMapping->Add(NewMeshManager->GetMyKey(), NewMeshManager);
				ProjectileNameToMeshManagerMapping->Add(ProjectileDefinitionId, NewMeshManager);
			}
		}
	}

	MeshManagerPtr = ProjectileNameToMeshManagerMapping->Find(ProjectileDefinitionId);

	if(MeshManagerPtr)
	{
		auto MeshManager = *MeshManagerPtr;
		if (MeshManager.IsValid())
		{
			FSkeletonKey NewProjectileKey = MeshManager->CreateNewInstance(WorldTransform, MuzzleVelocity, Layers::PROJECTILE, IsSensor);
			ProjectileKeyToMeshManagerMapping->Add(NewProjectileKey, MeshManager);
			return NewProjectileKey;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Could not find or load projectile instance manager with id %s"), *ProjectileDefinitionId.ToString());
	return FSkeletonKey();
}

void UArtilleryProjectileDispatch::DeleteProjectile(const FSkeletonKey Target)
{
	TWeakObjectPtr<AInstancedMeshManager> MeshManager;
	bool FoundKey = ProjectileKeyToMeshManagerMapping->RemoveAndCopyValue(Target, MeshManager);
	if (FoundKey && MeshManager.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DELETING"));
		MeshManager->CleanupInstance(Target);
		ProjectileKeyToMeshManagerMapping->Remove(Target);
	}
}

TWeakObjectPtr<AInstancedMeshManager> UArtilleryProjectileDispatch::GetProjectileMeshManagerByManagerKey(const FSkeletonKey ManagerKey)
{
	if (auto ManagerRefRef = ManagerKeyToMeshManagerMapping->Find(ManagerKey))
	{
		return *ManagerRefRef;
	}

	return nullptr;
}

TWeakObjectPtr<AInstancedMeshManager> UArtilleryProjectileDispatch::GetProjectileMeshManagerByProjectileKey(const FSkeletonKey ProjectileKey)
{
	if (auto ManagerRefRef = ProjectileKeyToMeshManagerMapping->Find(ProjectileKey))
	{
		return *ManagerRefRef;
	}

	return nullptr;
}

void UArtilleryProjectileDispatch::OnBarrageContactAdded(const BarrageContactEvent& ContactEvent)
{
	// We only care if one of the entities is a projectile
	if (ContactEvent.IsEitherEntityAProjectile())
	{
		auto ProjectileKey = ContactEvent.ContactEntity1.bIsProjectile ? ContactEvent.ContactEntity1.ContactKey : ContactEvent.ContactEntity2.ContactKey;
		auto EntityHitKey = ContactEvent.ContactEntity1.bIsProjectile ? ContactEvent.ContactEntity2.ContactKey : ContactEvent.ContactEntity1.ContactKey;

		UArtilleryDispatch* ArtilleryDispatch = GetWorld()->GetSubsystem<UArtilleryDispatch>();
		UTransformDispatch* TransformDispatch = GetWorld()->GetSubsystem<UTransformDispatch>();
		
		// TODO: make action based on projectile configuration, not just 100 health damage
		AttrPtr HitObjectHealthPtr = ArtilleryDispatch->GetAttrib(EntityHitKey, HEALTH);

		if (HitObjectHealthPtr.IsValid())
		{
			HitObjectHealthPtr->SetCurrentValue(HitObjectHealthPtr->GetCurrentValue() - 100);
		}

		auto HitActor = TransformDispatch->GetAActorByObjectKey(EntityHitKey);
		if (HitActor.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Hit a: %s"), *HitActor->GetFullName());
		}

		DeleteProjectile(ProjectileKey);
	}
	
}