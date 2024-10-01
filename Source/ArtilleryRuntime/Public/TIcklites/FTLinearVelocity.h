#pragma once
#include "Ticklite.h"
#include "ArtilleryDispatch.h"
#include "FArtilleryTicklitesThread.h"



//A ticklite's impl component(s) must provide:
//TICKLITE_StateReset on the memory block aspect
//TICKLITE_Calculate on the impl aspect
//TICKLITE_Apply on the impl aspect, consuming the memory block aspect's state
//TICKLITE_CoreReset on the impl aspect
//TICKLITE_CheckForExpiration on the impl aspect
//TICKLITE_OnExpiration

class FTLinearVelocity : public UArtilleryDispatch::TL_ThreadedImpl /*Facaded*/
{
public:
	FSkeletonKey VelocityTarget;
	VelocityVec PerTickVelocityToApply;
	uint32 TicksToSplitVelocityOver;
	uint32 TicksRemaining; //TODO: make this rollback correctly.
	VelocityVec Velocity;
	FTLinearVelocity(): TL_ThreadedImpl(), VelocityTarget(0), TicksToSplitVelocityOver(1), TicksRemaining(0)
	{
	}

	FTLinearVelocity(
		FSkeletonKey Target,
		VelocityVec Velocity,
		uint32 Duration
		) : VelocityTarget(Target), PerTickVelocityToApply(Velocity/Duration), TicksToSplitVelocityOver(Duration),
	TicksRemaining(Duration), Velocity(Velocity)
	{
	}
	void TICKLITE_StateReset()
	{
	}
	void TICKLITE_Calculate()
	{
	}
	//this isn't quite right. we should calculate the component in calculate
	//but for now, this is good enough for testing.
	void TICKLITE_Apply()
	{
		ArtilleryTime Now = this->GetShadowNow();
		--TicksRemaining;
		FBLet GameSimPhysicsObject = this->ADispatch->GetFBLetByObjectKey(VelocityTarget, Now);
		if(GameSimPhysicsObject)
		{
			FBarragePrimitive::ApplyForce(PerTickVelocityToApply, GameSimPhysicsObject);
		}
	}
	void TICKLITE_CoreReset()
	{
		TicksRemaining = TicksToSplitVelocityOver;
	}

	bool TICKLITE_CheckForExpiration()
	{
		return TicksRemaining == 0;
	}

	void TICKLITE_OnExpiration()
	{
		//no op
	}
};
//behold!
// FTLinearVelocity<FArtilleryTicklitesWorker*>& would allow you to use a reference indirection here, I believe.
typedef Ticklites::Ticklite<FTLinearVelocity> TL_LinearVelocity;
