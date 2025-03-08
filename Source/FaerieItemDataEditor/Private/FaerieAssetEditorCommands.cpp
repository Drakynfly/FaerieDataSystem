// Fill out your copyright notice in the Description page of Project Settings.

#include "FaerieAssetEditorCommands.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "FAssetEditorCommands"

namespace Faerie
{
	void FAssetEditorCommands::RegisterCommands()
	{
		UI_COMMAND(FocusViewport, "Focus Viewport", "Focus Viewport on Mesh", EUserInterfaceActionType::Button, FInputChord(EKeys::F));
	}
}

#undef LOCTEXT_NAMESPACE