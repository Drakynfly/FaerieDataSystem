// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieGuidFragment.generated.h"

/**
 * A simple fragment adding a FGuid onto an item.
 */
USTRUCT(BlueprintType)
struct FFaerieGuidFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	FFaerieGuidFragment() = default;
	FFaerieGuidFragment(const FGuid& Guid)
	  : Guid(Guid) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GuidFragment")
	FGuid Guid;

	bool InitializeRuntime(FMassEntityManager& EntityManager, const FMassEntityHandle Item);

	[[nodiscard]] UE_REWRITE friend uint32 GetTypeHash(const FFaerieGuidFragment& Value)
	{
		return GetTypeHash(Value.Guid);
	}

____FAERIE_FRAGMENT_DECL(FFaerieGuidFragment)
};

template <>
struct Faerie::ItemData::TMassTypeTraits<FFaerieGuidFragment> : TMassTypeTraitsBase<FFaerieGuidFragment>
{
	enum
	{
		PrimaryIdentifier = true
	};
};