// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreTypes.h"
#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"

#include "ArtilleryDispatch.h"
#include "FAttributeMap.h"
#include "TransformDispatch.h"
#include "UEnemyMachine.generated.h"

// So. This probably should share some functionality with UFireControlMachine, but I don't want to open that can of worms yet.
UCLASS()
class ARTILLERYRUNTIME_API UEnemyMachine : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UArtilleryDispatch* MyDispatch;
	UTransformDispatch* TransformDispatch;
	ActorKey ParentKey;

	// Me enemy, me want gun too
	// but me no implement now because me limit scope
	// TSet<FGunKey> MyGuns;

	//The direct presence of attributes in this way is likely obsoleted by switching to inheriting rather than friending
	//the UAS component, but I'm not totally sure.
	TSharedPtr<FAttributeMap> MyAttributes;

	bool Usable = false;
	
	//IF YOU DO NOT CALL THIS FROM THE GAMETHREAD, YOU WILL HAVE A BAD TIME.
	ActorKey CompleteRegistrationByActorParent(TMap<AttribKey, double> Attributes)
	{
		//these are initialized earlier under all intended orderings, but we cannot ensure that this function will be called correctly
		//so we should do what we can to foolproof things. As long as the world subsystems are up, force-updating
		//here will either:
		//work correctly
		//fail fast
		MyDispatch = GetWorld()->GetSubsystem<UArtilleryDispatch>();
		TransformDispatch =  GetWorld()->GetSubsystem<UTransformDispatch>();

		// Make a key yo
		auto keyHash = PointerHash(GetOwner());
		UE_LOG(LogTemp, Warning, TEXT("EnemyMachine Parented: %d"), keyHash);
		ParentKey = ActorKey(keyHash);
		Usable = true;

		// TODO: I have no guns and I am sad :( Give me gun in future

		MyAttributes = MakeShareable(new FAttributeMap(ParentKey, MyDispatch, Attributes));

		UE_LOG(LogTemp, Warning, TEXT("Enemy Mana: %f"), MyDispatch->GetAttrib(ParentKey, Attr::Mana)->GetCurrentValue());
		
		return ParentKey;
	}

	void InitializeComponent() override
	{
		Super::InitializeComponent();
		//we rely on attribute replication, which I think is borderline necessary, but I wonder if we should use effect replication.
		//historically, relying on gameplay effect replication has led to situations where key state was not managed through effects.
		//for OUR situation, where we have few attributes and many effects, huge amounts of effects are likely not interesting for us to replicate.
		ReplicationMode = EGameplayEffectReplicationMode::Minimal; 
	};

	//this happens post init but pre begin play, and the world subsystems should exist by this point.
	//we use this to help ensure that if the actor's begin play triggers first, things will be set correctly
	//I've left the same code in begin play as a fallback.
	void ReadyForReplication() override
	{
		Super::ReadyForReplication();
		MyDispatch = GetWorld()->GetSubsystem<UArtilleryDispatch>();
	}

	//on components, begin play can fire twice, because we aren't allowed to have nice things.
	//This can cause it to fire BEFORE the actor's begin play fires, which leaves you with
	//very few good options. the bool Usable helps control this.
	//This is, ironically, not a problem in actual usage, only testing, for us.
	void BeginPlay() override
	{
		Super::BeginPlay(); 
		MyDispatch = GetWorld()->GetSubsystem<UArtilleryDispatch>();
	};

	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override
	{
		Super::OnComponentDestroyed(bDestroyingHierarchy);
	};
};