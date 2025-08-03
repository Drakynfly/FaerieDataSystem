// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieCardSettings.h"
#include "Misc/App.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardSettings)

FName UFaerieCardSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}