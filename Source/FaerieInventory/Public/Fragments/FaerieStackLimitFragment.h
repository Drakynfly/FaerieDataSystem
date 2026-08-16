// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieStackLimitFragment.generated.h"

USTRUCT(Blueprintable)
struct FFaerieStackLimitFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	// Max stack size
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0))
	int32 MaxStackSize = 0;

	int32 GetStackLimit() const;
};

namespace Faerie::Container
{
	FAERIEINVENTORY_API int32 GetItemStackLimit(const FMassEntityManager* EntityManager, TValid<const FFaerieItemInstance&> Item);
}