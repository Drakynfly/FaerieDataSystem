// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"

namespace Faerie::ItemMesh
{
    class FModule : public IModuleInterface
    {
    public:
        virtual void StartupModule() override;
        virtual void ShutdownModule() override;
    };
}

