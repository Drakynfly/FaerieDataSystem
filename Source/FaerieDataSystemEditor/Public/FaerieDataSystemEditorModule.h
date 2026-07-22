// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "PropertyEditorDelegates.h"
#include "Modules/ModuleInterface.h"
#include "Styling/SlateStyle.h"

namespace Faerie::Editor
{
    static void StaticUnregisterCustomizations(const TConstArrayView<FName>& ClassNames, const TConstArrayView<FName>& PropertyTypeNames);
}

class FAERIEDATASYSTEMEDITOR_API IFaerieDataSystemEditorModuleBase : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    /** IModuleInterface implementation */

protected:
    void OnPostEngineInit();

    void RegisterCustomizations(const TMap<FName, FOnGetDetailCustomizationInstance>& ClassCustomizations, const TMap<FName, FOnGetPropertyTypeCustomizationInstance>& PropertyTypeCustomizations);

public:
    void AddPropertyTypeCustomization(FName Name, FOnGetPropertyTypeCustomizationInstance LayoutDelegate);
    void RemovePropertyTypeCustomization(FName Name);

private:
    /** Detail Customization keys; Cached so they can be unregistered */
    TArray<FName> CustomizedClassNames;

    /** Property Customization keys; Cached so they can be unregistered */
    TArray<FName> CustomizedPropertyTypeNames;
};

class FFaerieDataSystemEditorModule final : public IModuleInterface
{
public:
    FFaerieDataSystemEditorModule();

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    /** IModuleInterface implementation */

private:
    /** Editor style set; Cached so it can be unregistered */
    TSharedPtr<FSlateStyleSet> StyleSet;
};