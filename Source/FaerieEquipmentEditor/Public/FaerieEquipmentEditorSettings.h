// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FaerieEquipmentEditorSettings.generated.h"

class UDurabilityGradeAsset;

/**
 *
 */
UCLASS(Config = EditorPerProjectUserSettings, defaultconfig, meta = (DisplayName = "Faerie Equipment Editor"))
class FAERIEEQUIPMENTEDITOR_API UFaerieEquipmentEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

#if WITH_EDITOR
	TArray<FString> GetDebugInfoForCCM(const float CCM) const;
#endif

protected:
	UPROPERTY(EditAnywhere, Config, Category = "Durability Info")
	TArray<TSoftObjectPtr<UDurabilityGradeAsset>> EditorInfoGradeList;
};