// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "skeletonize.h"
#include "SkeletonTypes.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Templates/SubclassOf.h"
#include "UObject/UnrealType.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
#include <bitset>
#include "Containers/CircularBuffer.h"
#include "FGunKey.generated.h"


USTRUCT(BlueprintType)
struct FGunKey
{
	GENERATED_BODY()
public:
	//TODO: this needs to be removed. we should never allow a default gunkey.
	FGunKey()
	{}
	FGunKey(FString Name, uint64_t id):
	GunDefinitionID(Name), GunInstanceID(id)
	{
	}
	UPROPERTY(BlueprintReadOnly)
	FString GunDefinitionID; //this will need to be human searchable
	//FUN STORY: BLUEPRINT CAN'T USE UINT64.
	uint64 GunInstanceID;
	//while actor key has a different behavior, gunkey only applies the mask when switching up to objectkey.
	//this is because those types are interchangeable for legacy reasons, which I intend to eliminate.
	operator FSkeletonKey() const
	{
		return FSkeletonKey(FORGE_SKELETON_KEY(GetTypeHash(GunDefinitionID) + GetTypeHash(GunInstanceID), SKELLY::SFIX_ART_GUNS));
	}
	friend uint32 GetTypeHash(const FGunKey& Other)
	{
		// it's probably fine!
		return GetTypeHash(Other.GunDefinitionID) + GetTypeHash(Other.GunInstanceID);
	}
};
static bool operator==(FGunKey const& lhs, FGunKey const& rhs) {
	return (lhs.GunDefinitionID == rhs.GunDefinitionID) && (lhs.GunInstanceID == rhs.GunInstanceID);
}
//when sorted, gunkeys follow their instantiation order!
static bool operator<(FGunKey const& lhs, FGunKey const& rhs) {
	return (lhs.GunInstanceID < rhs.GunInstanceID);
}