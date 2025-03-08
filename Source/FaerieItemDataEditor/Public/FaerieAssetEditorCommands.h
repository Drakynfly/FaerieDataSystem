// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
