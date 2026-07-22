// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryContentEditorModule.h"
#include "PropertyEditorModule.h"
#include "GameplayTagsEditorModule.h"
#include "GridLayout/SpatialTypes.h"
#include "Customizations/ItemCapacityCustomization.h"
#include "Customizations/ItemShapeCustomization.h"
#include "Extensions/InventoryMetadataExtension.h"
#include "Extensions/InventoryUserdataExtension.h"
#include "Capacity/FaerieCapacityHelper.h"

#define LOCTEXT_NAMESPACE "FaerieInventoryContentEditorModule"

void FFaerieInventoryContentEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	TMap<FName, FOnGetDetailCustomizationInstance> ClassCustomizations;
	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	StructCustomizations.Add(FFaerieInventoryMetaTag::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameplayTagCustomizationPublic::MakeInstance));
	StructCustomizations.Add(FFaerieInventoryUserTag::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameplayTagCustomizationPublic::MakeInstance));
	StructCustomizations.Add(FFaerieWeightEditor::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FInventoryWeightCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieWeightEditor_Float::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FInventoryWeightCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieItemCapacity::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemCapacityCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieGridShape::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemShapeCustomization::MakeInstance));

	RegisterCustomizations(ClassCustomizations, StructCustomizations);
}

void FFaerieInventoryContentEditorModule::ShutdownModule()
{
	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieInventoryContentEditorModule, FaerieInventoryContentEditor)