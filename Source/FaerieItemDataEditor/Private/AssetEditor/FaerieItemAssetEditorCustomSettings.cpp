// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieItemAssetEditorCustomSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemAssetEditorCustomSettings)

void UFaerieItemAssetEditorCustomSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FEditorCustomSettingsEventData Data;
	Data.PropertyEvent = &PropertyChangedEvent;

	EditorCustomSettingsEvent.ExecuteIfBound(Data);
}

void UFaerieItemAssetEditorCustomSettings::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	FEditorCustomSettingsEventData Data;
	Data.PropertyEvent = &PropertyChangedEvent;
	Data.ChainPropertyEvent = &PropertyChangedEvent;

	EditorCustomSettingsEvent.ExecuteIfBound(Data);
}
