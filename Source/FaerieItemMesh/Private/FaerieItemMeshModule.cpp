// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemMeshModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FaerieItemMeshModule"

namespace Faerie::ItemMesh
{
	void FModule::StartupModule()
    {
    }

    void FModule::ShutdownModule()
    {
    }
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(Faerie::ItemMesh::FModule, FaerieItemMesh)