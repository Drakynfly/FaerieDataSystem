// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FaerieItemDataSettings.generated.h"

class UMassEntityConfigAsset;

/**
 * 
 */
UCLASS(Config = "Project", defaultconfig)
class FAERIEITEMDATA_API UFaerieItemDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings implementation
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings implementation

	UPROPERTY(EditAnywhere, Config, Category = "Mass")
	TSoftObjectPtr<UMassEntityConfigAsset> ItemDataMassConfig;
};
