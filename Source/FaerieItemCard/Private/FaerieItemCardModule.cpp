// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCardModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FaerieItemCardModule"

DEFINE_LOG_CATEGORY(LogFaerieItemCard)

void FFaerieItemCardModule::StartupModule()
{
}

void FFaerieItemCardModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieItemCardModule, FaerieItemCard)