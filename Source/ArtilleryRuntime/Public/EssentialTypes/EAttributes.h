#pragma once
#include "CoreMinimal.h"
#include "ConservedAttribute.h"
#include <array>

#include "ConservedKey.h"
#include "EAttributes.generated.h"

UENUM(BlueprintType, Blueprintable)
enum class E_AttribKey : uint8
{
	// Entity Attributes
	Speed,
	Health,
	MaxHealth,
	HealthRechargePerTick,
	Shields,
	MaxShields,
	ShieldsRechargePerTick,
	Mana,
	MaxMana,
	ManaRechargePerTick,
	TicksTilJumpAvailable,
	JumpHeight,
	ProposedDamage,

	// Gun Attributes
	Ammo,
	MaxAmmo,
	FireCooldown,
	FireCooldownRemaining,
	ReloadTime,
	ReloadTimeRemaining,
	Range,
	TicksSinceLastFired,
	LastFiredTimestamp,
};

UENUM(BlueprintType, Blueprintable)
enum class E_IdentityAttrib : uint8
{
	Target,
	EquippedMainGun,
	EquippedSecondaryGun,
	EquippedMoveAbility,
	EquippedDefAbility,
	EquippedDashAbility
};

namespace Arty
{
	//gonna need to ditch the stupid attributes from gameplay or rework them
	//all because of the bloody Base value meaning that we might have 2 doubles per.
	//if you want a base, add a base.
	//Attributes is a UE namespace, so we gotta call this attributeslist. sigh.

	namespace AttributesList{

	}
	//MANA should always be granted in multiples of 10 since 10m/t is our standard recharge.
	using AttribKey = E_AttribKey;
	using Attr = AttribKey;
	using Ident = E_IdentityAttrib;

	// Entity attribs
	constexpr AttribKey HEALTH = Arty::AttribKey::Health;
	constexpr AttribKey MAX_HEALTH = Arty::AttribKey::MaxHealth;
	constexpr AttribKey MANA = Arty::AttribKey::Mana;
	constexpr AttribKey DASH_CURRENCY = MANA;
	constexpr AttribKey MAX_MANA = Arty::AttribKey::MaxMana;
	constexpr AttribKey PROPOSED_DAMAGE = Arty::AttribKey::ProposedDamage;

	// Gun Attribs
	constexpr AttribKey AMMO = Arty::AttribKey::Ammo;
	constexpr AttribKey MAX_AMMO = Arty::AttribKey::MaxAmmo;\
	constexpr AttribKey COOLDOWN = Arty::AttribKey::FireCooldown;
	constexpr AttribKey COOLDOWN_REMAINING = Arty::AttribKey::FireCooldownRemaining;
	constexpr AttribKey RELOAD = Arty::AttribKey::ReloadTime;
	constexpr AttribKey RELOAD_REMAINING = Arty::AttribKey::ReloadTimeRemaining;
	constexpr AttribKey TICKS_SINCE_GUN_LAST_FIRED = Arty::AttribKey::TicksSinceLastFired;
	typedef TSharedPtr<FConservedAttributeData> AttrPtr;
	typedef TSharedPtr<FConservedAttributeKey> IdentPtr;
	typedef TMap<AttribKey, AttrPtr> AttributeMap;
	typedef TMap<Ident, IdentPtr> IdentityMap;
	typedef TSharedPtr<AttributeMap> AttrMapPtr;
	typedef TSharedPtr<IdentityMap> IdMapPtr;


}
