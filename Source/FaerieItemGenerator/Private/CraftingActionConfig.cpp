﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CraftingActionConfig.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CraftingActionConfig)

UCraftingActionConfig::UCraftingActionConfig()
{
	Squirrel = CreateDefaultSubobject<USquirrel>(FName{TEXTVIEW("Squirrel")});
}