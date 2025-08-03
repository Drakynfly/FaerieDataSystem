// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"
#include "Templates/SharedPointer.h"

class FUICommandInfo;

namespace Faerie
{
	class FAssetEditorCommands : public TCommands<FAssetEditorCommands>
	{
	public:
		FAssetEditorCommands()
			: TCommands<FAssetEditorCommands>("AssetEditorEditor",
				NSLOCTEXT("Contexts", "FaerieItemAssetEditor", "Faerie Item Asset Editor"),
				NAME_None, FAppStyle::GetAppStyleSetName()) {}

		/** Focuses Viewport on Mesh */
		TSharedPtr<FUICommandInfo> FocusViewport;

		/** Initialize commands */
		virtual void RegisterCommands() override;
	};
}
