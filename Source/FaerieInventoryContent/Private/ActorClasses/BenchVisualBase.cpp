// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ActorClasses/BenchVisualBase.h"

#include "BenchBehaviorBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BenchVisualBase)

ABenchVisualBase::ABenchVisualBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

void ABenchVisualBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Behavior);
}

void ABenchVisualBase::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Behavior))
	{
		if (HasAuthority())
		{
			AddReplicatedSubObject(Behavior);
			Behavior->InitializeNetObject(this);
		}

		Behavior->BeginPlay();
	}
}