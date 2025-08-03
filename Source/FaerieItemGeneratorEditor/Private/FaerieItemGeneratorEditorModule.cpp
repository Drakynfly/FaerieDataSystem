// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemGeneratorEditorModule.h"
#include "Modules/ModuleManager.h"

#include "ItemCraftingConfig.h"
#include "ItemGeneratorConfig.h"
#include "ItemUpgradeConfig.h"
#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "Customizations/ItemGenerationConfigCustomization.h"
#include "Customizations/ItemsArrayCustomization.h"
#include "Customizations/OnTheFlyConfigCustomization.h"
#include "Customizations/TableDropCustomization.h"
#include "Customizations/WeightedDropCustomization.h"

#define LOCTEXT_NAMESPACE "FaerieItemGeneratorEditorModule"

void FFaerieItemGeneratorEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	TMap<FName, FOnGetDetailCustomizationInstance> ClassCustomizations;
	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	ClassCustomizations.Add(UItemGenerationConfig::StaticClass()->GetFName(),
	FOnGetDetailCustomizationInstance::CreateStatic(&FItemGenerationConfigCustomization::MakeInstance));

	StructCustomizations.Add(FTableDrop::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FTableDropCustomization::MakeInstance));
	StructCustomizations.Add(FWeightedDrop::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWeightedDropCustomization::MakeInstance));
	StructCustomizations.Add(FOnTheFlyItemCraftingConfig::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FOnTheFlyConfigCustomization::MakeInstance));
	StructCustomizations.Add(FOnTheFlyItemUpgradeConfig::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FOnTheFlyConfigCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieWeightedDropPool::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemsArrayCustomization::MakeInstance));

	RegisterDetailCustomizations(ClassCustomizations);
	RegisterPropertyCustomizations(StructCustomizations);
}

void FFaerieItemGeneratorEditorModule::ShutdownModule()
{
	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieItemGeneratorEditorModule, FaerieItemGeneratorEditor)