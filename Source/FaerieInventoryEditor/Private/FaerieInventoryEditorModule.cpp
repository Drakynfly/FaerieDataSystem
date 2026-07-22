// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryEditorModule.h"
#include "Editor.h"
#include "FaerieDataUtilsModule.h"
#include "FaerieInventoryTag.h"
#include "FaerieItemContainerStructs.h"
#include "GameplayTagsEditorModule.h"
#include "FaerieStorageStructs.h"
#include "Customizations/SimpleInlineHeaderStructCustomization.h"

#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "FaerieInventoryEditorModule"

void FFaerieInventoryEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	StructCustomizations.Add(FFaerieEntryKey::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSimpleInlineHeaderStructCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieStackKey::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSimpleInlineHeaderStructCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieInventoryTag::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameplayTagCustomizationPublic::MakeInstance));

	RegisterCustomizations({}, StructCustomizations);

	FFaerieDataUtilsModule& DataUtilsModule = FModuleManager::GetModuleChecked<FFaerieDataUtilsModule>("FaerieDataUtils");
	DataUtilsModule.OnAskEditorToOpenObjectEditorWindow.BindStatic(&FFaerieInventoryEditorModule::OpenObjectEditorWindow);
}

void FFaerieInventoryEditorModule::ShutdownModule()
{
	if (FFaerieDataUtilsModule* DataUtilsModule = FModuleManager::GetModulePtr<FFaerieDataUtilsModule>("FaerieDataUtils"))
	{
		DataUtilsModule->OnAskEditorToOpenObjectEditorWindow.Unbind();
	}

	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

void FFaerieInventoryEditorModule::OpenObjectEditorWindow(const TNotNull<UObject*> Object)
{
	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		AssetEditorSubsystem->OpenEditorForAsset(Object);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieInventoryEditorModule, FaerieInventoryEditor)