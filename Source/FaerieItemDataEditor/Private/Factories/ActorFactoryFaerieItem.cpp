// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ActorFactoryFaerieItem.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieMeshSettings.h"
#include "ActorClasses/FaerieItemOwningActorBase.h"
#include "Tokens/FaerieVisualActorClassToken.h"

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
		const UFaerieItem* Item = ItemAsset->GetEditorItemView();
		if (IsValid(Item))
		{
			const UFaerieVisualActorClassToken* VisualActorToken = Item->GetToken<UFaerieVisualActorClassToken>();
			if (IsValid(VisualActorToken))
			{
				return VisualActorToken->LoadActorClassSynchronous();
			}
		}

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

	const FFaerieItemStack Stack
	{
		ItemAsset->GetItemInstance(EFaerieItemInstancingMutability::Automatic),
		1
	};

	if (AFaerieItemOwningActorBase* Visual = Cast<AFaerieItemOwningActorBase>(NewActor))
	{
		FEditorScriptExecutionGuard ScriptGuard;
		Visual->Possess(Stack);
	}
}

#undef LOCTEXT_NAMESPACE
