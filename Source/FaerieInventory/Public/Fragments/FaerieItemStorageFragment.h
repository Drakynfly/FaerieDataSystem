// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerBase.h"
#include "FaerieMassFragment.h"
#include "FaerieItemStorage.h"

#include "FaerieItemStorageFragment.generated.h"

class UFaerieItemContainerBase;
class UFaerieItemStackContainer;
class UFaerieItemStorage;

USTRUCT()
struct FFaerieInlineItemStorage
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, Instanced, NoClear, Category = "InlineStorage")
	TObjectPtr<UFaerieItemStorage> Storage;
};

USTRUCT()
struct FFaerieItemStorageFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, Category = "ItemStorageFragment")
	FFaerieInlineItemStorage Storage;

#if WITH_EDITOR
	EDataValidationResult IsDataValid(FDataValidationContext& Context) const;
#endif

	bool InitializeRuntime(FMassEntityManager& EntityManager, const FFaerieItemInstance& Instance);

____FAERIE_FRAGMENT_DECL(FFaerieItemStorageFragment)
};

template<>
struct TMassFragmentTraits<FFaerieItemStorageFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};

template <>
struct Faerie::ItemData::TMassFragmentTypeTraits<FFaerieItemStorageFragment> : TMassFragmentTypeTraitsBase<FFaerieItemStorageFragment>
{
	enum
	{
		RequiresMutable = true
	};
};

USTRUCT()
struct FFaerieInlineStackContainer
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, Instanced, NoClear, Category = "InlineStackContainer")
	TObjectPtr<UFaerieItemStackContainer> Stack;
};

USTRUCT()
struct FFaerieChildStackFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	/*
	 * Children slots of an item. There can currently be up to five of these.
	 */
	UPROPERTY(EditInstanceOnly, Category = "ChildSlot")
	TArray<FFaerieInlineStackContainer> Slots;

	bool InitializeRuntime(FMassEntityManager& EntityManager, const FFaerieItemInstance& Instance);

____FAERIE_FRAGMENT_DECL(FFaerieChildStackFragment)
};

template<>
struct TMassFragmentTraits<FFaerieChildStackFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};

template <>
struct Faerie::ItemData::TMassFragmentTypeTraits<FFaerieChildStackFragment> : TMassFragmentTypeTraitsBase<FFaerieChildStackFragment>
{
	enum
	{
		RequiresMutable = true
	};
};