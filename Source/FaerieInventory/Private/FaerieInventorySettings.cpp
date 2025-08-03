// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventorySettings.h"
#include "Misc/App.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieInventorySettings)

FName UFaerieInventorySettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}