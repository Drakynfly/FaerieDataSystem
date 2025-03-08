// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataEditorModule.h"
#include "FaerieAssetEditorCommands.h"
#include "FaerieItemDataFilter.h"

#include "FaerieItemSource.h"
#include "PropertyEditorModule.h"
#include "Customizations/OnTheFlyConfigCustomization.h"
#include "Customizations/FaerieItemSourceObjectCustomization.h"

#define LOCTEXT_NAMESPACE "FaerieItemDataEditorModule"

void FFaerieItemDataEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	Faerie::FAssetEditorCommands::Register();

	ToolbarExtensibilityManager = MakeShared<FExtensibilityManager>();

	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	StructCustomizations.Add(FFaerieItemSourceObject::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FFaerieItemSourceObjectCustomization::MakeInstance));
	StructCustomizations.Add(FInlineFaerieItemDataFilter::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FOnTheFlyConfigCustomization::MakeInstance));

	RegisterPropertyCustomizations(StructCustomizations);
}

void FFaerieItemDataEditorModule::ShutdownModule()
{
	Faerie::FAssetEditorCommands::Unregister();

	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieItemDataEditorModule, FaerieItemDataEditor)