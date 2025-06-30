// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieDataSystemEditorModule.h"

#include "PropertyEditorModule.h"
#include "PropertyEditorDelegates.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Customizations/SGameplayTagGraphPin_FIXED.h"

DEFINE_LOG_CATEGORY(LogFaerieDataSystemEditorModule);

#define LOCTEXT_NAMESPACE "FaerieDataSystemEditorModule"

void IFaerieDataSystemEditorModuleBase::StartupModule()
{
    FCoreDelegates::OnPostEngineInit.AddRaw(this, &IFaerieDataSystemEditorModuleBase::OnPostEngineInit);
}

void IFaerieDataSystemEditorModuleBase::ShutdownModule()
{
    FCoreDelegates::OnPostEngineInit.RemoveAll(this);

    if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
    {
        auto&& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

        // Unregister detail customizations
        for (auto&& Customization : DetailCustomizations)
        {
            PropertyModule.UnregisterCustomClassLayout(Customization);
        }

        // Unregister property customizations
        for (auto&& Customization : PropertyCustomizations)
        {
            PropertyModule.UnregisterCustomPropertyTypeLayout(Customization);
        }

        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

void IFaerieDataSystemEditorModuleBase::OnPostEngineInit()
{
    // @note: this is a patch to fix a bug in Unreal as of 5.4/5.5 where gameplay tag filters are evaluated in the wrong order
    TSharedPtr<FGameplayTagsGraphPanelPinFactory_ForFix> GameplayTagsGraphPanelPinFactoryFixed = MakeShared<FGameplayTagsGraphPanelPinFactory_ForFix>();
    FEdGraphUtilities::RegisterVisualPinFactory(GameplayTagsGraphPanelPinFactoryFixed);
}

void IFaerieDataSystemEditorModuleBase::RegisterDetailCustomizations(
    const TMap<FName, FOnGetDetailCustomizationInstance>& Customizations)
{
    auto&& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    for (auto&& Element : Customizations)
    {
        PropertyModule.RegisterCustomClassLayout(Element.Key, Element.Value);
        DetailCustomizations.Add(Element.Key);
    }

    PropertyModule.NotifyCustomizationModuleChanged();
}

void IFaerieDataSystemEditorModuleBase::RegisterPropertyCustomizations(
    const TMap<FName, FOnGetPropertyTypeCustomizationInstance>& Customizations)
{
    auto&& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

    for (auto&& Element : Customizations)
    {
        PropertyModule.RegisterCustomPropertyTypeLayout(Element.Key, Element.Value);
        PropertyCustomizations.Add(Element.Key);
    }

    PropertyModule.NotifyCustomizationModuleChanged();
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