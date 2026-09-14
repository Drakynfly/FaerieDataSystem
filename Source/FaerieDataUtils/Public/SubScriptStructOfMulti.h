// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Templates/SubScriptStructOf.h"
#include "SubScriptStructOfMulti.generated.h"

/*
 * A variant of SubScriptStructOf that gets customized in the editor to allow one of any base struct.
 */
USTRUCT()
struct FSubScriptStructOfMulti : public FSubScriptStructOf
{
	GENERATED_BODY()

	using FSubScriptStructOf::FSubScriptStructOf;
};
