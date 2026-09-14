// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FaerieInventorySettings.generated.h"

/**
 * Empty place holder.
 */
UCLASS(config = Project, defaultconfig, meta = (DisplayName = "Faerie Inventory"))
class FAERIEINVENTORY_API UFaerieInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings implementation
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings implementation
};