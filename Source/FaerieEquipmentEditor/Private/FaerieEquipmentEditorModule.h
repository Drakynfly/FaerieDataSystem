// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieDataSystemEditorModule.h"

class FFaerieEquipmentEditorModule : public IFaerieDataSystemEditorModuleBase
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};