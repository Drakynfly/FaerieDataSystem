// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actors/FaerieItemVisualBase.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemVisualBase)

AFaerieItemVisualBase::AFaerieItemVisualBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

USceneComponent* AFaerieItemVisualBase::GetDefaultAttachComponent() const
{
	USceneComponent* MeshComponent = GetDefaultMeshComponent();
	if (IsValid(MeshComponent))
	{
		return MeshComponent;
	}
	return Super::GetDefaultAttachComponent();
}

void AFaerieItemVisualBase::BeginPlay()
{
	Super::BeginPlay();

	RegenerateDataDisplay();
}

void AFaerieItemVisualBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearDataDisplay();

	Super::EndPlay(EndPlayReason);
}

TOptional<FFaerieItemInstance> AFaerieItemVisualBase::GetItemInstance() const
	PURE_VIRTUAL(AFaerieItemVisualBase::GetItemInstance, return NullOpt; )

Faerie::ItemData::FProxyChangeEvent::RegistrationType& AFaerieItemVisualBase::GetOnProxyChangeEvent()
{
	checkNoEntry();
	static Faerie::ItemData::FProxyChangeEvent Blank;
	return Blank;
}

void AFaerieItemVisualBase::ClearDataDisplay_Implementation()
{
}

void AFaerieItemVisualBase::DisplayData_Implementation()
{
}

void AFaerieItemVisualBase::NotifyDisplayDataFinished(const bool Success)
{
	OnDisplayFinished.Broadcast(Success);
}

void AFaerieItemVisualBase::RegenerateDataDisplay()
{
	ClearDataDisplay();

	if (GetItemInstance().IsSet())
	{
		DisplayData();
	}
}