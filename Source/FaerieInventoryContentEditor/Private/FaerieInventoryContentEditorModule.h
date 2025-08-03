// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieDataSystemEditorModule.h"

class FFaerieInventoryContentEditorModule : public IFaerieDataSystemEditorModuleBase
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};