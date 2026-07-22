// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ActorFactoryFaerieItem.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemStackContainer.h"
#include "FaerieMeshSettings.h"
#include "Actors/FaerieItemOwningActorBase.h"
#include "Fragments/FaerieActorFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ActorFactoryFaerieItem)

#define LOCTEXT_NAMESPACE "ActorFactoryFaerieItem"

UActorFactoryFaerieItem::UActorFactoryFaerieItem()
{
	DisplayName = LOCTEXT("FaerieItemDisplayName", "Faerie Item");
}

bool UActorFactoryFaerieItem::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid() || !AssetData.IsInstanceOf(UFaerieItemAsset::StaticClass()))
	{
		OutErrorMsg = INVTEXT("Invalid AssetData!");
		return false;
	}

	return IsValid(GetDefaultActorClass(AssetData));
}

UClass* UActorFactoryFaerieItem::GetDefaultActorClass(const FAssetData& AssetData)
{
	if (auto&& ItemAsset = Cast<UFaerieItemAsset>(AssetData.GetAsset()))
	{
		auto ActorClassFragment = Faerie::ItemData::GetDefaultFragment<FFaerieActorFragment>(ItemAsset->GetAssetTemplateItem());
		if (ActorClassFragment.IsValid())
		{
			if (TSubclassOf<AFaerieItemOwningActorBase> OwningActorClass = ActorClassFragment->LoadOwningActorClassSynchronous())
			{
				return OwningActorClass;
			}
		}

		// Note: Editor use of LoadSync is allowed here.
		return GetDefault<UFaerieMeshSettings>()->DefaultPickupActor.LoadSynchronous();
	}

	return nullptr;
}

bool UActorFactoryFaerieItem::CanPlaceElementsFromAssetData(const FAssetData& InAssetData)
{
	return InAssetData.IsValid() && InAssetData.IsInstanceOf(UFaerieItemAsset::StaticClass());
}

void UActorFactoryFaerieItem::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	const UFaerieItemAsset* ItemAsset = Cast<UFaerieItemAsset>(Asset);
	if (!IsValid(ItemAsset))
	{
		return;
	}

	if (AFaerieItemOwningActorBase* OwningActor = CastChecked<AFaerieItemOwningActorBase>(NewActor))
	{
		OwningActor->SourceAsset = ItemAsset;
		OwningActor->StackCopies = 1;
		OwningActor->RegenerateStack();
	}
}

#undef LOCTEXT_NAMESPACE
