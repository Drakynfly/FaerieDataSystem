// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieDataSystemEditorModule.h"

class FFaerieItemGeneratorEditorModule : public IFaerieDataSystemEditorModuleBase
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void RegisterMutatorType(FName StructName);
    void UnregisterMutatorType(FName StructName);

private:
    // Cache the mutator customization instance that we will use for any statically registered mutator subtypes.
    FOnGetPropertyTypeCustomizationInstance MutatorTypeCustomizationInstance;
};