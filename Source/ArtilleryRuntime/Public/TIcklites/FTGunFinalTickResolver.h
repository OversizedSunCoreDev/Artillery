
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
	class TLGunFinalTickResolver : public UArtilleryDispatch::TL_ThreadedImpl /*Facaded*/
	{
	public:
		FSkeletonKey EntityKey;
		TLGunFinalTickResolver(): TL_ThreadedImpl()
		{
		}

		TLGunFinalTickResolver(
			FSkeletonKey Target
			) : TL_ThreadedImpl(), EntityKey(Target)
		{
		}
		void TICKLITE_StateReset()
		{
		}
		void TICKLITE_Calculate()
		{
		}
		
		void TICKLITE_Apply()
		{
			// handle gun cooldown (fire rate)
			auto GunCooldownRemaining = TL_ThreadedImpl::ADispatch->GetAttrib(EntityKey,  COOLDOWN_REMAINING);

			// Reduce cd remaining by one tick, flooring at 0.
			if (GunCooldownRemaining.IsValid())
			{
				auto CurrCooldownValue = GunCooldownRemaining->GetCurrentValue();
				if (CurrCooldownValue > 0.f) {
					GunCooldownRemaining->SetCurrentValue(std::max(CurrCooldownValue - 1, 0.f));
				}
			}

			// handle reload
			auto CurrentAmmo = TL_ThreadedImpl::ADispatch->GetAttrib(EntityKey,  AMMO);
			auto MaxAmmo = TL_ThreadedImpl::ADispatch->GetAttrib(EntityKey,  MAX_AMMO);
			auto ReloadTime = TL_ThreadedImpl::ADispatch->GetAttrib(EntityKey,  RELOAD);
			auto ReloadRemaining = TL_ThreadedImpl::ADispatch->GetAttrib(EntityKey,  RELOAD_REMAINING);
			if (MaxAmmo.IsValid() && CurrentAmmo.IsValid())
			{
				auto CurrentAmmoValue = CurrentAmmo->GetCurrentValue();
				auto MaxAmmoValue = MaxAmmo->GetCurrentValue();
				auto ReloadTimeValue = ReloadTime->GetCurrentValue();
				auto ReloadRemainingValue = ReloadRemaining->GetCurrentValue();
				if (MaxAmmoValue > 0.f)
				{
					// Only process reload system if this gun uses ammo + ammo is at zero

					if (ReloadRemainingValue > -1.f)
					{
						// Tick reload timer if its greater than -1, stopping at -1
						ReloadRemaining->SetCurrentValue(ReloadRemainingValue - 1);
					}

					if (ReloadRemainingValue == 0)
					{
						// Complete the reload
						CurrentAmmo->SetCurrentValue(MaxAmmoValue);
					}
				
					if (CurrentAmmoValue <= 0.f && ReloadRemainingValue <= -1.f)
					{
						// If we are out of ammo and the reload remaining is negative, start the reload
						ReloadRemaining->SetCurrentValue(ReloadTimeValue);
					}
				}
			}
		}
		
		void TICKLITE_CoreReset()
		{
		}

		bool TICKLITE_CheckForExpiration()
		{
			return false; //add check for aliveness of ya owner, factor that down.
		}

		void TICKLITE_OnExpiration()
		{
			//no op
		}
	};

typedef Ticklites::Ticklite<TLGunFinalTickResolver> GunFinalTickResolver;

