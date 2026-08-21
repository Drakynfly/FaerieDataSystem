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

	bool InitializeRuntime(TNotNull<UObject*> Outer, const FFaerieItemInstance& Instance);

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieGuidFragment& Value)
	{
		return GetTypeHash(Value.Guid);
	}
};

template <>
struct Faerie::ItemData::TMassFragmentTypeTraits<FFaerieGuidFragment> : TMassFragmentTypeTraitsBase<FFaerieGuidFragment>
{
	enum
	{
		PrimaryIdentifier = true
	};
};