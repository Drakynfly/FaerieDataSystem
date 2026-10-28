// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "FaerieMassFragment.h"
#include "GameplayTagContainer.h"
#include "FaerieReferenceFragment.generated.h"

class UFaerieItemAsset;

// DEPRECATED: REMOVE ME
USTRUCT()
struct FFaerieTaggedReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "TaggedReference")
	FGameplayTag Tag;

	// @this being a TObjectPtr requires the fragment to be NotTriviallyCopyable.
	// To resolve this, this would need to be either TWeakObjectPtr, or TSoftObjectPtr,
	// but then what would keep these loaded...
	UPROPERTY(EditAnywhere, Category = "TaggedReference")
	TObjectPtr<const UFaerieItemAsset> Reference;
};

// DEPRECATED: REMOVE ME
USTRUCT()
struct FFaerieReferenceFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ReferenceFragment", meta = (ForceInlineRow))
	FFaerieTaggedReference References[3];

	const UFaerieItem* GetReferencedItem(FGameplayTag ReferenceTag, bool MatchExact) const;

____FAERIE_FRAGMENT_DECL(FFaerieReferenceFragment)
};
