// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actors/ItemRepresentationActor.h"
#include "FaerieItem.h"

AItemRepresentationActor::AItemRepresentationActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

USceneComponent* AItemRepresentationActor::GetDefaultAttachComponent() const
{
	USceneComponent* MeshComponent = GetDefaultMeshComponent();
	if (IsValid(MeshComponent))
	{
		return MeshComponent;
	}
	return Super::GetDefaultAttachComponent();
}

void AItemRepresentationActor::Destroyed()
{
	ClearDataDisplay();
	Super::Destroyed();
}

const UFaerieItem* AItemRepresentationActor::GetItemObject() const
{
	return DataSource.GetItemObject();
}

int32 AItemRepresentationActor::GetCopies() const
{
	return DataSource.GetCopies();
}

TScriptInterface<IFaerieItemOwnerInterface> AItemRepresentationActor::GetItemOwner() const
{
	return DataSource.GetOwner();
}

void AItemRepresentationActor::ClearDataDisplay_Implementation()
{
}

void AItemRepresentationActor::DisplayData_Implementation()
{
}

void AItemRepresentationActor::NotifyDisplayDataFinished(const bool Success)
{
	OnDisplayFinished.Broadcast(Success, this);
}

void AItemRepresentationActor::RegenerateDataDisplay()
{
	ClearDataDisplay();

	if (IsValid(DataSource.GetItemObject()))
	{
		DisplayData();
	}
}

void AItemRepresentationActor::SetSourceProxy(const FFaerieItemProxy Source)
{
	if (Source != DataSource)
	{
		DataSource = Source;
		RegenerateDataDisplay();
	}
}