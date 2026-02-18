// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemGeneratorEditorModule.h"
#include "Modules/ModuleManager.h"

#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "Customizations/ItemGenerationConfigCustomization.h"
#include "Customizations/ItemsArrayCustomization.h"
#include "Customizations/TableDropCustomization.h"
#include "Customizations/WeightedDropCustomization.h"
#include "Generation/FaerieItemGenerationConfig.h"

#define LOCTEXT_NAMESPACE "FaerieItemGeneratorEditorModule"

void FFaerieItemGeneratorEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	TMap<FName, FOnGetDetailCustomizationInstance> ClassCustomizations;
	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	ClassCustomizations.Add(UFaerieItemGenerationConfig::StaticClass()->GetFName(),
	FOnGetDetailCustomizationInstance::CreateStatic(&FItemGenerationConfigCustomization::MakeInstance));

	StructCustomizations.Add(FFaerieTableDrop::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&Faerie::GeneratorEditor::FTableDropCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieWeightedDrop::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&Faerie::GeneratorEditor::FWeightedDropCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieWeightedPool::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&Faerie::Editor::FItemsArrayCustomization::MakeInstance));

	RegisterDetailCustomizations(ClassCustomizations);
	RegisterPropertyCustomizations(StructCustomizations);
}

void FFaerieItemGeneratorEditorModule::ShutdownModule()
{
	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieItemGeneratorEditorModule, FaerieItemGeneratorEditor)