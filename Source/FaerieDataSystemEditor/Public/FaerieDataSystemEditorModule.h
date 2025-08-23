// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "PropertyEditorDelegates.h"
#include "Modules/ModuleInterface.h"
#include "Styling/SlateStyle.h"

class FAERIEDATASYSTEMEDITOR_API IFaerieDataSystemEditorModuleBase : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    /** IModuleInterface implementation */

protected:
    void OnPostEngineInit();

    void RegisterDetailCustomizations(const TMap<FName, FOnGetDetailCustomizationInstance>& Customizations);
    void RegisterPropertyCustomizations(const TMap<FName, FOnGetPropertyTypeCustomizationInstance>& Customizations);

private:
    /** Property Customization keys; Cached so they can be unregistered */
    TSet<FName> PropertyCustomizations;

    /** Detail Customization keys; Cached so they can be unregistered */
    TSet<FName> DetailCustomizations;
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