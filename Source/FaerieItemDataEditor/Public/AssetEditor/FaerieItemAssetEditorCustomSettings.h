// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemAssetEditorCustomSettings.generated.h"

struct FEditorCustomSettingsEventData
{
	FPropertyChangedEvent* PropertyEvent = nullptr;
	FPropertyChangedChainEvent* ChainPropertyEvent = nullptr;
};

using FEditorCustomSettingsEvent = TDelegate<void(const FEditorCustomSettingsEventData&)>;

/**
 * 
 */
UCLASS(Config = Editor)
class FAERIEITEMDATAEDITOR_API UFaerieItemAssetEditorCustomSettings : public UObject
{
	GENERATED_BODY()

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere, config, transient, Category = "Token Data")
	bool ShowCapacityBounds = false;

	FEditorCustomSettingsEvent::RegistrationType& GetOnSettingsChanged() { return EditorCustomSettingsEvent; }

private:
	FEditorCustomSettingsEvent EditorCustomSettingsEvent;
};
