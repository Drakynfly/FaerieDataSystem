// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "FaerieItemFactories.generated.h"

class UFaerieItemAsset;

UCLASS()
class FAERIEITEMDATAEDITOR_API UFaerieItemAsset_Factory : public UFactory
{
    GENERATED_BODY()

public:
    UFaerieItemAsset_Factory(const FObjectInitializer& ObjectInitializer);

    virtual bool ConfigureProperties() override;
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    // The template to duplicate
    UPROPERTY(EditAnywhere, Category = "Config")
    TObjectPtr<UFaerieItemAsset> Template = nullptr;
};

UCLASS()
class FAERIEITEMDATAEDITOR_API UFaerieItemTemplate_Factory : public UFactory
{
    GENERATED_BODY()

public:
    UFaerieItemTemplate_Factory(const FObjectInitializer& ObjectInitializer);

    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};