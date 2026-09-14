// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemGeneratorEditorModule.h"
#include "FaerieItemMutator.h"

#include "Modules/ModuleManager.h"

#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "Customizations/ItemGenerationConfigCustomization.h"
#include "Customizations/ItemMutatorCustomization.h"
#include "Customizations/ItemsArrayCustomization.h"
#include "Customizations/TableDropCustomization.h"
#include "Customizations/WeightedDropCustomization.h"
#include "Generation/FaerieItemGenerationConfig.h"

#define LOCTEXT_NAMESPACE "FaerieItemGeneratorEditorModule"

using namespace Faerie;

void FFaerieItemGeneratorEditorModule::StartupModule()
{
	IFaerieDataSystemEditorModuleBase::StartupModule();

	TMap<FName, FOnGetDetailCustomizationInstance> ClassCustomizations;
	TMap<FName, FOnGetPropertyTypeCustomizationInstance> StructCustomizations;

	ClassCustomizations.Add(UFaerieItemGenerationConfig::StaticClass()->GetFName(),
	FOnGetDetailCustomizationInstance::CreateStatic(&FItemGenerationConfigCustomization::MakeInstance));

	StructCustomizations.Add(FFaerieTableDrop::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&GeneratorEditor::FTableDropCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieWeightedDrop::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&GeneratorEditor::FWeightedDropCustomization::MakeInstance));
	StructCustomizations.Add(FFaerieWeightedPool::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&Editor::FItemsArrayCustomization::MakeInstance));

	MutatorTypeCustomizationInstance = FOnGetPropertyTypeCustomizationInstance::CreateStatic(&GeneratorEditor::FItemMutatorCustomization::MakeInstance);

	Generation::FModule* GenerationModule = static_cast<Generation::FModule*>(FModuleManager::Get().LoadModule("FaerieItemGenerator"));
	GenerationModule->Editor_AddMutatorType.BindLambda([this](const TNotNull<const UScriptStruct*> StructType)
	{
		this->RegisterMutatorType(StructType->GetFName());
	});
	GenerationModule->Editor_RemoveMutatorType.BindLambda([this](const TNotNull<const UScriptStruct*> StructType)
	{
		this->UnregisterMutatorType(StructType->GetFName());
	});

	RegisterCustomizations(ClassCustomizations, StructCustomizations);
}

void FFaerieItemGeneratorEditorModule::ShutdownModule()
{
	if (Generation::FModule* GenerationModule = FModuleManager::Get().GetModulePtr<Generation::FModule>("FaerieItemGenerator"))
	{
		GenerationModule->Editor_AddMutatorType.Unbind();
		GenerationModule->Editor_RemoveMutatorType.Unbind();
	}

	IFaerieDataSystemEditorModuleBase::ShutdownModule();
}

void FFaerieItemGeneratorEditorModule::RegisterMutatorType(const FName StructName)
{
	AddPropertyTypeCustomization(StructName, MutatorTypeCustomizationInstance);
}

void FFaerieItemGeneratorEditorModule::UnregisterMutatorType(const FName StructName)
{
	RemovePropertyTypeCustomization(StructName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieItemGeneratorEditorModule, FaerieItemGeneratorEditor)