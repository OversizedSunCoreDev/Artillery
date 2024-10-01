// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/UnrealType.h"
#include "Engine/DataTable.h"
#include "AttributeSet.h"
#include "SkeletonTypes.h"
#include "Containers/CircularBuffer.h"
#include "ConservedKey.generated.h"
/**
 * Conserved key attributes record their last 128 changes.
 * Currently, this is for debug purposes, but it will be necessary for rollback.
 */
USTRUCT(BlueprintType)
struct ARTILLERYRUNTIME_API FConservedAttributeKey
{
	GENERATED_BODY()
	TCircularBuffer<FSkeletonKey> CurrentHistory = TCircularBuffer<FSkeletonKey>(128);
	TCircularBuffer<FSkeletonKey> RemoteHistory = TCircularBuffer<FSkeletonKey>(128);
	TCircularBuffer<FSkeletonKey> BaseHistory = TCircularBuffer<FSkeletonKey>(128);

	UPROPERTY(BlueprintReadOnly, Category = "Attribute")
	FSkeletonKey BaseValue;

	UPROPERTY(BlueprintReadOnly, Category = "Attribute")
	FSkeletonKey CurrentValue;

	void SetCurrentValue(FSkeletonKey NewValue) {
		CurrentHistory[CurrentHistory.GetNextIndex(CurrentHead)] = CurrentValue;
		CurrentValue = NewValue;
		++CurrentHead;
	};
	
	
	void SetRemoteValue(FSkeletonKey NewValue) {
		RemoteHistory[RemoteHistory.GetNextIndex(RemoteHead)] = NewValue;
		++RemoteHead;
	};
	

	void SetBaseValue(FSkeletonKey NewValue) {
		BaseHistory[BaseHistory.GetNextIndex(BaseHead)] = BaseValue;
		BaseValue = NewValue;
		++BaseHead;
	};

protected:
	uint64_t BaseHead = 0;
	uint64_t CurrentHead = 0;
	uint64_t RemoteHead = 0;
};

