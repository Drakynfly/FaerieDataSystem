// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "FaerieMassFragment.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "FaerieReferenceFragment.generated.h"

class UFaerieItemAsset;

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

USTRUCT()
struct FFaerieReferenceFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ReferenceFragment", meta = (ForceInlineRow))
	FFaerieTaggedReference References[3];

	static const UFaerieItemAsset* GetReferencedAsset(const FMassEntityManager* EntityManager, const FFaerieItemInstance& Item, FGameplayTag ReferenceTag, bool MatchExact);
	static const UFaerieItem* GetReferencedItem(const FMassEntityManager* EntityManager, const FFaerieItemInstance& Item, FGameplayTag ReferenceTag, bool MatchExact);

	UE_REWRITE TConstArrayView<FFaerieTaggedReference> GetReferences() const { return References; }

	const UFaerieItemAsset* GetReferencedAsset(FGameplayTag ReferenceTag, bool MatchExact) const;

	const UFaerieItem* GetReferencedItem(FGameplayTag ReferenceTag, bool MatchExact) const;

____FAERIE_FRAGMENT_DECL(FFaerieReferenceFragment)
};

// @Todo i guess we need this for TObjectPtr...
template<>
struct TMassFragmentTraits<FFaerieReferenceFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};