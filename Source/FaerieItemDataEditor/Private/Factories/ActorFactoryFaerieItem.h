// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ActorFactories/ActorFactory.h"
#include "ActorFactoryFaerieItem.generated.h"

/**
 * Handles drag-and-drop of a FaerieItem asset into the level from content browser to spawn a Visual Actor.
 */
UCLASS()
class FAERIEITEMDATAEDITOR_API UActorFactoryFaerieItem : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryFaerieItem();

	//~ UActorFactory
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual UClass* GetDefaultActorClass(const FAssetData& AssetData) override;
	virtual bool CanPlaceElementsFromAssetData(const FAssetData& InAssetData) override;

	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	//~ UActorFactory
};
