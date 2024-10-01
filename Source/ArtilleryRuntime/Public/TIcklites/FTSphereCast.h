#pragma once

#include <functional>
#include "Ticklite.h"
#include "ArtilleryDispatch.h"
#include "FArtilleryTicklitesThread.h"
#include "FWorldSimOwner.h"

class FTSphereCast : public UArtilleryDispatch::TL_ThreadedImpl
{
	uint32 TicksRemaining;
	FBarrageKey ShapeCastSourceObject;
	float Radius;
	float Distance;
	FVector RayStart;
	FVector RayDirection;
	TSharedPtr<FHitResult> HitResultPtr;
	std::function<void(FVector, TSharedPtr<FHitResult>)> Callback;

public:
	FTSphereCast() : TicksRemaining(2), ShapeCastSourceObject(0), Radius(0.01), Distance(5000), Callback(nullptr)
	{
		HitResultPtr = MakeShared<FHitResult>();
	}

	FTSphereCast(
		FBarrageKey ShapeCastSource,
		float SphereRadius,
		float CastDistance,
		const FVector& StartLocation,
		const FVector& Direction,
		const std::function<void(FVector, TSharedPtr<FHitResult>)> CallbackFunc
		)
		: TicksRemaining(1),
		  ShapeCastSourceObject(ShapeCastSource),
		  Radius(SphereRadius), Distance(CastDistance),
		  RayStart(StartLocation),
		  RayDirection(Direction),
	      Callback(CallbackFunc)
	{
		HitResultPtr = MakeShared<FHitResult>();
	}

	void TICKLITE_StateReset()
	{
	}

	void TICKLITE_Calculate()
	{
		UBarrageDispatch* Physics = this->ADispatch->DispatchOwner->GetWorld()->GetSubsystem<UBarrageDispatch>();
		if (Physics)
		{
			Physics->SphereCast(ShapeCastSourceObject, Radius, Distance, RayStart, RayDirection, HitResultPtr);

			if (Callback && HitResultPtr->MyItem != JPH::BodyID::cInvalidBodyID)
			{
				Callback(RayStart, HitResultPtr);
			}
		}
	}

	void TICKLITE_Apply()
	{
		--TicksRemaining;
	}

	void TICKLITE_CoreReset()
	{
	}

	bool TICKLITE_CheckForExpiration()
	{
		return TicksRemaining == 0;
	}

	void TICKLITE_OnExpiration()
	{
	}
};

using TL_SphereCast = Ticklites::Ticklite<FTSphereCast>;
