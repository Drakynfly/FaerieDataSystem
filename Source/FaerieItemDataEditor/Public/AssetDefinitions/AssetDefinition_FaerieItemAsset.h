// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "AssetDefinitionDefault.h"
#include "FaerieDataSystemGlobals.h"
#include "FaerieItemAsset.h"
#include "AssetDefinition_FaerieItemAsset.generated.h"

/**
 *
 */
UCLASS()
class FAERIEITEMDATAEDITOR_API UAssetDefinition_FaerieItemAsset : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override
	{
		return NSLOCTEXT("AssetDefinitions", "FaerieItemAsset", "Faerie Item Asset");
	}
	virtual FText GetAssetDescription(const FAssetData& AssetData) const override
	{
		return NSLOCTEXT("AssetDefinitions", "FaerieItemAsset_Desc", "An asset to create a simple faerie item definition.");
	}
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(127, 0, 155)); } // Nice purple color
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UFaerieItemAsset::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		return Faerie::Editor::DefaultCategories;
	}
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	// UAssetDefinition End
};
