// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetDefinitions/AssetDefinition_FaerieItemAsset.h"
#include "AssetEditor/FaerieItemAssetEditor.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetDefinition_FaerieItemAsset)

EAssetCommandResult UAssetDefinition_FaerieItemAsset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UFaerieItemAsset* ItemAsset : OpenArgs.LoadObjects<UFaerieItemAsset>())
	{
		const TSharedRef<Faerie::Ed::FItemAssetEditorToolkit> NewEditor = MakeShared<Faerie::Ed::FItemAssetEditorToolkit>();
		NewEditor->InitAssetEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, ItemAsset);
	}

	return EAssetCommandResult::Handled;
}

UThumbnailInfo* UAssetDefinition_FaerieItemAsset::LoadThumbnailInfo(const FAssetData& InAssetData) const
{
	return UE::Editor::FindOrCreateThumbnailInfo(InAssetData.GetAsset(), USceneThumbnailInfo::StaticClass());
}
