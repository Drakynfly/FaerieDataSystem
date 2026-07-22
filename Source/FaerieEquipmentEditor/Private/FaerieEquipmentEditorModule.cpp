// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentEditorModule.h"
#include "FaerieSlotTag.h"
#include "GameplayTagsEditorModule.h"

#define LOCTEXT_NAMESPACE "FaerieEquipmentEditorModule"

void FFaerieEquipmentEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	StructCustomizations.Add(FFaerieSlotTag::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameplayTagCustomizationPublic::MakeInstance));

	RegisterCustomizations({}, StructCustomizations);
}

void FFaerieEquipmentEditorModule::ShutdownModule()
{
	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieEquipmentEditorModule, FaerieEquipmentEditor)