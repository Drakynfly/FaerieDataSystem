// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieAssetEditorCommands.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "FAssetEditorCommands"

namespace Faerie::Editor
{
	void FAssetEditorCommands::RegisterCommands()
	{
		UI_COMMAND(FocusViewport, "Focus Viewport", "Focus Viewport on Mesh", EUserInterfaceActionType::Button, FInputChord(EKeys::F));
	}
}

#undef LOCTEXT_NAMESPACE