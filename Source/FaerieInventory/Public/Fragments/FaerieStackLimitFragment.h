// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieStackLimitFragment.generated.h"

namespace Faerie::ItemData
{
	struct FReference;
}

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
	FAERIEINVENTORY_API int32 GetItemStackLimit(const ItemData::FOptionalEntityManager& EntityManager, const ItemData::FReference& Item);
}