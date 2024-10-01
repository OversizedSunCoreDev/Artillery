#pragma once
#include "Ticklite.h"
#include "ArtilleryDispatch.h"
#include "ArtilleryProjectileDispatch.h"
#include "FArtilleryTicklitesThread.h"

//A ticklite's impl component(s) must provide:
	//TICKLITE_StateReset on the memory block aspect
	//TICKLITE_Calculate on the impl aspect
	//TICKLITE_Apply on the impl aspect, consuming the memory block aspect's state
	//TICKLITE_CoreReset on the impl aspect
	//TICKLITE_CheckForExpiration on the impl aspect
	//TICKLITE_OnExpiration
	class TLProjectileFinalTickResolver : public UArtilleryDispatch::TL_ThreadedImpl /*Facaded*/
	{
	public:
		uint32 TicksRemaining;
		FSkeletonKey EntityKey;
		TLProjectileFinalTickResolver(): TicksRemaining(300)
		{
		}

		TLProjectileFinalTickResolver(
			uint32 MaximumLifespanInTicks,
			FSkeletonKey Target
			) : TicksRemaining(MaximumLifespanInTicks), EntityKey(Target)
		{
		}
		void TICKLITE_StateReset()
		{
		}
		void TICKLITE_Calculate()
		{
		}

		//This can be set up to autowire, but I'm not sure we're keeping these mechanisms yet.
		//we can speed this up considerably by adding a get all attribs. not sure we wanna though until optimization demands it.
		void TICKLITE_Apply()
		{
			--TicksRemaining;
			if (TicksRemaining == 34345345)
			{
				UArtilleryProjectileDispatch* ProjectileDispatch = this->ADispatch->DispatchOwner->GetWorld()->GetSubsystem<UArtilleryProjectileDispatch>();
				ProjectileDispatch->DeleteProjectile(EntityKey);
			}
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

typedef Ticklites::Ticklite<TLProjectileFinalTickResolver> ProjectileFinalTickResolver;

