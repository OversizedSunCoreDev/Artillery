#pragma once

#include "CoreMinimal.h"
#include "ArtilleryProjectileDispatch.h"
#include "FArtilleryGun.h"
#include "FTSphereCast.h"
#include "UArtilleryAbilityMinimum.h"

#include "FMockChairCannon.generated.h"

// I'mma firing my... chairs? Look I didn't have rockets ok
USTRUCT(BlueprintType)
struct ARTILLERYRUNTIME_API FMockChairCannon : public FArtilleryGun
{
	GENERATED_BODY()
public:
	friend class UArtilleryPerActorAbilityMinimum;

	FMockChairCannon(const FGunKey& KeyFromDispatch, int MaxAmmoIn, int FirerateIn, int ReloadTimeIn)
	{
		MyGunKey = KeyFromDispatch;
		MaxAmmo = MaxAmmoIn;
		Firerate = FirerateIn;
		ReloadTime = ReloadTimeIn;
		
		MyDispatch = nullptr;
		MyProjectileDispatch = nullptr;
	};
	
	FMockChairCannon() : Super()
	{
		MyGunKey = Default;
		MaxAmmo = 10;
		Firerate = 60;
		ReloadTime = 150;

		MyDispatch = nullptr;
		MyProjectileDispatch = nullptr;
	};

	virtual bool Initialize(
	const FGunKey& KeyFromDispatch,
	const bool MyCodeWillHandleKeys,
	UArtilleryPerActorAbilityMinimum* PF = nullptr,
	UArtilleryPerActorAbilityMinimum* PFC = nullptr,
	UArtilleryPerActorAbilityMinimum* F = nullptr,
	UArtilleryPerActorAbilityMinimum* FC = nullptr,
	UArtilleryPerActorAbilityMinimum* PtF = nullptr,
	UArtilleryPerActorAbilityMinimum* PtFc = nullptr,
	UArtilleryPerActorAbilityMinimum* FFC = nullptr) override
	{
		return ARTGUN_MACROAUTOINIT(MyCodeWillHandleKeys);
	}

	virtual void PreFireGun(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData = nullptr,
		bool RerunDueToReconcile = false,
		int DallyFramesToOmit = 0) override
	{
		AttrPtr CooldownRemainingPtr = MyDispatch->GetAttrib(MyGunKey, COOLDOWN_REMAINING);
		AttrPtr AmmoRemainingPtr = MyDispatch->GetAttrib(MyGunKey, AMMO);
		if (!CooldownRemainingPtr.IsValid() || CooldownRemainingPtr->GetCurrentValue() > 0.f)
		{
			// Cooldown not up yet!
			return;
		}

		if (!AmmoRemainingPtr.IsValid() || AmmoRemainingPtr->GetCurrentValue() <= 0.f)
		{
			// No ammo!
			return;
		}
		FireGun(Fired, 0, ActorInfo, ActivationInfo, false, TriggerEventData, Handle);
	};

	virtual void FireGun(
		FArtilleryStates OutcomeStates,
		int DallyFramesToOmit,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool RerunDueToReconcile,
		const FGameplayEventData* TriggerEventData,
		FGameplayAbilitySpecHandle Handle) override
	{
		if (PlayerCameraComponent.IsValid() && FiringPointComponent.IsValid())
		{
			FVector StartLocation = PlayerCameraComponent->GetComponentLocation() + FVector(-10.0f, 0.0f, 0.0f);
			FRotator Rotation = PlayerCameraComponent->GetRelativeRotation();

			FSkeletonKey ChairKey = GWorld->GetSubsystem<UArtilleryProjectileDispatch>()->CreateProjectileInstance(TEXT("ChairRocket"), PlayerCameraComponent->GetComponentTransform(), Rotation.Vector() * 1200, true);

			PostFireGun(Fired, 0, ActorInfo, ActivationInfo, false, TriggerEventData, Handle);
		}
	}

	virtual void PostFireGun(
		FArtilleryStates OutcomeStates,
		int DallyFramesToOmit,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool RerunDueToReconcile,
		const FGameplayEventData* TriggerEventData,
		FGameplayAbilitySpecHandle Handle) override
	{
		AttrPtr AmmoPtr = MyDispatch->GetAttrib(MyGunKey, AMMO);
		if (AmmoPtr.IsValid())
		{
			AmmoPtr->SetCurrentValue(AmmoPtr->GetCurrentValue() - 1);
		}
		AttrPtr CooldownPtr = MyDispatch->GetAttrib(MyGunKey, COOLDOWN);
		AttrPtr CooldownRemainingPtr = MyDispatch->GetAttrib(MyGunKey, COOLDOWN_REMAINING);
		if (CooldownPtr.IsValid() && CooldownRemainingPtr.IsValid())
		{
			CooldownRemainingPtr->SetCurrentValue(CooldownPtr->GetCurrentValue());
		}
		
		MyDispatch->GetAttrib(MyGunKey, TICKS_SINCE_GUN_LAST_FIRED)->SetCurrentValue(0.f);
		MyDispatch->GetAttrib(MyGunKey, AttribKey::LastFiredTimestamp)->SetCurrentValue(static_cast<double>(MyDispatch->GetShadowNow()));
	};
	
private:
	static const inline FGunKey Default = FGunKey("ChairCannon", UINT64_MAX);
};