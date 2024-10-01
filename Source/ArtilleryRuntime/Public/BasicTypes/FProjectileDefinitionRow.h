#pragma once
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Engine/DataTable.h"

#include "FProjectileDefinitionRow.generated.h"

USTRUCT(BlueprintType)
struct FProjectileDefinitionRow : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileDefinition)
	FString ProjectileDefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileDefinition)
	FString ProjectileMeshLocation;
};
