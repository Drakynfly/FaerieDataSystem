// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemGenerationBench.h"
#include "Squirrel.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemGenerationBench)

UItemGenerationBench::UItemGenerationBench()
{
	Squirrel = CreateDefaultSubobject<USquirrel>(FName{TEXTVIEW("Squirrel")});
}