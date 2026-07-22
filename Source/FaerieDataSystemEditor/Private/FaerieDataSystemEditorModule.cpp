// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieDataSystemEditorModule.h"

#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Customizations/SGameplayTagGraphPin_FIXED.h"

#define LOCTEXT_NAMESPACE "FaerieDataSystemEditorModule"

namespace Faerie::Editor
{
    constexpr const TCHAR* PropertyEditorModuleName = TEXT("PropertyEditor");

    void StaticUnregisterCustomizations(const TConstArrayView<FName>& ClassNames, const TConstArrayView<FName>& PropertyTypeNames)
    {
        if (FModuleManager::Get().IsModuleLoaded(PropertyEditorModuleName))
        {
            FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditorModuleName);

            // Unregister all classes customized by name
            for (auto&& ClassName : ClassNames)
            {
                PropertyModule.UnregisterCustomClassLayout(ClassName);
            }

            // Unregister all structures
            for (auto&& PropertyTypeName : PropertyTypeNames)
            {
                PropertyModule.UnregisterCustomPropertyTypeLayout(PropertyTypeName);
            }

            PropertyModule.NotifyCustomizationModuleChanged();
        }
    }
}

using namespace Faerie;

void IFaerieDataSystemEditorModuleBase::StartupModule()
{
    FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &IFaerieDataSystemEditorModuleBase::OnPostEngineInit);
}

void IFaerieDataSystemEditorModuleBase::ShutdownModule()
{
    FCoreDelegates::GetOnPostEngineInit().RemoveAll(this);

    // Unregister customizations
    Editor::StaticUnregisterCustomizations(CustomizedClassNames, CustomizedPropertyTypeNames);
}

void IFaerieDataSystemEditorModuleBase::OnPostEngineInit()
{
    UE_CALL_ONCE([]()
    {
        // @note: this is a patch to fix a bug in Unreal as of 5.4/5.5 where gameplay tag filters are evaluated in the wrong order
        TSharedPtr<FGameplayTagsGraphPanelPinFactory_ForFix> GameplayTagsGraphPanelPinFactoryFixed = MakeShared<FGameplayTagsGraphPanelPinFactory_ForFix>();
        FEdGraphUtilities::RegisterVisualPinFactory(GameplayTagsGraphPanelPinFactoryFixed);
    });
}

void IFaerieDataSystemEditorModuleBase::RegisterCustomizations(
    const TMap<FName, FOnGetDetailCustomizationInstance>& ClassCustomizations,
    const TMap<FName, FOnGetPropertyTypeCustomizationInstance>& PropertyTypeCustomizations)
{
    auto&& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(Editor::PropertyEditorModuleName);

    for (auto&& ClassCustomization : ClassCustomizations)
    {
        PropertyModule.RegisterCustomClassLayout(ClassCustomization.Key, ClassCustomization.Value);
        CustomizedClassNames.Add(ClassCustomization.Key);
    }

    for (auto&& PropertyTypeCustomization : PropertyTypeCustomizations)
    {
        PropertyModule.RegisterCustomPropertyTypeLayout(PropertyTypeCustomization.Key, PropertyTypeCustomization.Value);
        CustomizedPropertyTypeNames.Add(PropertyTypeCustomization.Key);
    }

    PropertyModule.NotifyCustomizationModuleChanged();
}

void IFaerieDataSystemEditorModuleBase::AddPropertyTypeCustomization(const FName Name, FOnGetPropertyTypeCustomizationInstance LayoutDelegate)
{
    auto&& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(Editor::PropertyEditorModuleName);

    PropertyModule.RegisterCustomPropertyTypeLayout(Name, MoveTemp(LayoutDelegate));
    CustomizedPropertyTypeNames.Add(Name);

    PropertyModule.NotifyCustomizationModuleChanged();
}

void IFaerieDataSystemEditorModuleBase::RemovePropertyTypeCustomization(const FName Name)
{
    if (FModuleManager::Get().IsModuleLoaded(Editor::PropertyEditorModuleName))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(Editor::PropertyEditorModuleName);

        PropertyModule.UnregisterCustomPropertyTypeLayout(Name);
        CustomizedPropertyTypeNames.Remove(Name);

        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

FFaerieDataSystemEditorModule::FFaerieDataSystemEditorModule()
{
    StyleSet = MakeShared<FSlateStyleSet>(TEXT("FaerieDataSystemStyle"));

    // Content path of this plugin
    const FString ContentDir = IPluginManager::Get().FindPlugin("FaerieDataSystem")->GetBaseDir();

    // The image we wish to load is located inside the Resources folder inside the Base Directory
    // so let's set the content dir to the base dir and manually switch to the Resources folder:
    StyleSet->SetContentRoot(ContentDir);

    const FVector2D Size128 = FVector2D(128.f, 128.f);
    const TCHAR* ExtPng = TEXT(".png");

    TMap<FString, FString> PathAssetPairs;
    //PathAssetPairs.Add("IconInventory128", "FaerieItemAsset");
    PathAssetPairs.Add("IconTable128", "FaerieItemPool");

    FSlateImageBrush* ThumbnailBrush;

    for (auto&& i : PathAssetPairs)
    {
        // Create a brush from the icon
        // ReSharper disable once CppJoinDeclarationAndAssignment
        ThumbnailBrush = new FSlateImageBrush(StyleSet->RootToContentDir("Resources/" + i.Key, ExtPng), Size128);
        if (ThumbnailBrush)
        {
            // In order to bind the thumbnail to our class we need to type ClassThumbnail.X where X is the name of the C++ class of the asset
            StyleSet->Set(*("ClassThumbnail." + i.Value), ThumbnailBrush);
        }
    }
}

void FFaerieDataSystemEditorModule::StartupModule()
{
    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FFaerieDataSystemEditorModule::ShutdownModule()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(StyleSet->GetStyleSetName());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFaerieDataSystemEditorModule, FaerieDataSystemEditor);