// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryEditorModule.h"
#include "FaerieInventoryTag.h"
#include "GameplayTagsEditorModule.h"
#include "InventoryDataStructs.h"
#include "Customizations/SimpleInlineHeaderStructCustomization.h"

#define LOCTEXT_NAMESPACE "FaerieInventoryEditorModule"

void FFaerieInventoryEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	StructCustomizations.Add(FEntryKey::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSimpleInlineHeaderStructCustomization::MakeInstance));
	StructCustomizations.Add(FStackKey::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSimpleInlineHeaderStructCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieInventoryTag::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameplayTagCustomizationPublic::MakeInstance));

	RegisterPropertyCustomizations(StructCustomizations);
}

void FFaerieInventoryEditorModule::ShutdownModule()
{
	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieInventoryEditorModule, FaerieInventoryEditor)