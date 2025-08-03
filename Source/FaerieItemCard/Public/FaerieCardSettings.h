// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemCardTags.h"
#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"
#include "FaerieCardSettings.generated.h"

class UFaerieCardBase;

/**
 *
 */
UCLASS(Config = "Project", defaultconfig)
class FAERIEITEMCARD_API UFaerieCardSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings implementation
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings implementation

	// Disable to prevent UFaerieCardSubsystem being created
	UPROPERTY(Config, EditAnywhere, Category = "Generator")
	bool CreateCardGeneratorPlayerSubsystems = true;

	UE_DEPRECATED(5.5, "Use FallbackClasses instead")
	UPROPERTY(Config, EditAnywhere, Category = "Classes")
	TMap<TSubclassOf<class UFaerieItemCardToken>, TSoftClassPtr<UFaerieCardBase>> DefaultClasses;

	// Item Card classes to use when an item doesn't specify one.
	UPROPERTY(Config, EditAnywhere, Category = "Classes", meta = (ForceInlineRow))
	TMap<FFaerieItemCardType, TSoftClassPtr<UFaerieCardBase>> FallbackClasses;
};