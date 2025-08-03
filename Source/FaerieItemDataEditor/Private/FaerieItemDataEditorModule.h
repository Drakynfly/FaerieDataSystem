// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieDataSystemEditorModule.h"

class FExtensibilityManager;

class FFaerieItemDataEditorModule : public IFaerieDataSystemEditorModuleBase
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    TSharedPtr<FExtensibilityManager> GetEditorToolbarExtensibilityManager() { return ToolbarExtensibilityManager; }

private:
    TSharedPtr<FExtensibilityManager> ToolbarExtensibilityManager;
};