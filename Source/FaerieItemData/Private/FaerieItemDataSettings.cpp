// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataSettings.h"

#include "Misc/App.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataSettings)

FName UFaerieItemDataSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}
