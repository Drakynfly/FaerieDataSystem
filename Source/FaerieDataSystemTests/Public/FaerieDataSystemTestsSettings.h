// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FaerieDataSystemTestsSettings.generated.h"

/**
 * 
 */
UCLASS(Config = EditorPerProjectUserSettings, defaultconfig, meta = (DisplayName = "Faerie Data System Tests"))
class FAERIEDATASYSTEMTESTS_API UFaerieDataSystemTestsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings implementation
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings implementation

	UPROPERTY(EditAnywhere, config, Category = "Serialization Test", meta = (AllowedClasses = "/Script/FaerieItemData.FaerieItemAsset"))
	FSoftObjectPath TestImmutableItemAsset;

	UPROPERTY(EditAnywhere, config, Category = "Serialization Test", meta = (AllowedClasses = "/Script/FaerieItemData.FaerieItemAsset"))
	FSoftObjectPath TestMutableItemAsset;
};
