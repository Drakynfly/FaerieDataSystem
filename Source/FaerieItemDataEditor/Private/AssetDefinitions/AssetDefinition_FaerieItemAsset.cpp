// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetDefinitions/AssetDefinition_FaerieItemAsset.h"
#include "AssetEditor/FaerieItemAssetEditor.h"

EAssetCommandResult UAssetDefinition_FaerieItemAsset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UFaerieItemAsset* ItemAsset : OpenArgs.LoadObjects<UFaerieItemAsset>())
	{
		const TSharedRef<FFaerieItemAssetEditor> NewEditor = MakeShared<FFaerieItemAssetEditor>();
		NewEditor->InitAssetEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, ItemAsset);
	}

	return EAssetCommandResult::Handled;
}
