// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieDataUtilsModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FaerieDataUtilsModule"

void FFaerieDataUtilsModule::StartupModule()
{
}

void FFaerieDataUtilsModule::ShutdownModule()
{
}

#if WITH_EDITOR
void FFaerieDataUtilsModule::AskEditorToOpenObjectEditorWindow(const TNotNull<UObject*> Object) const
{
	if (OnAskEditorToOpenObjectEditorWindow.IsBound())
	{
		OnAskEditorToOpenObjectEditorWindow.Execute(Object);
	}
}
#endif

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieDataUtilsModule, FaerieDataUtils)