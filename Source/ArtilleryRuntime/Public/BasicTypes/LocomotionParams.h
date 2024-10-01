// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/UnrealType.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
#include "Containers/CircularBuffer.h"
#include "BristleconeCommonTypes.h"
#include "ArtilleryCommonTypes.h"
#include "ArtilleryShell.h"

struct LocomotionParams
{
	uint64_t time;
	FSkeletonKey parent;
	FArtilleryShell previousIndex; // may NOT be current -1. :/
	FArtilleryShell currentIndex;

	LocomotionParams(uint64_t time, uint64_t parent, FArtilleryShell prev, FArtilleryShell cur):
		time(time),
		parent(parent),
		previousIndex(prev),
		currentIndex(cur)
	{
	}
};

//this creates a stable sub-ordering that ensures deterministic sequence of operations.
static bool operator<(LocomotionParams const& lhs, LocomotionParams const& rhs)
{
	return lhs.time == rhs.time ? (lhs.time < rhs.time) : (lhs.parent < rhs.parent);
}

namespace Arty
{
	typedef TArray<LocomotionParams> MovementBuffer;
	typedef TTripleBuffer<MovementBuffer> BufferedMoveEvents;
	
}
