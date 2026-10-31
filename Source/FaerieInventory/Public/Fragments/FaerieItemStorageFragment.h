// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieMassTypeTraits.h"
#include "FaerieItemStorageFragment.generated.h"

class UFaerieItemStackContainer;
class UFaerieItemStorage;

USTRUCT()
struct FFaerieInlineItemStorage
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, Instanced, NoClear, Category = "InlineStorage")
	TObjectPtr<UFaerieItemStorage> Storage;
};

/*
 * Fragment to store an item storage object inside an item (eg, a backpack).
 * Implemented as a SharedFragment to keep the storage object alive, as regular fragments are not memory-managed.
 * SaveData is exported as a FFaerieStorageExportData instance.
 */
USTRUCT()
struct FFaerieItemStorageFragment : public FFaerieMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, Category = "ItemStorageFragment")
	FFaerieInlineItemStorage Storage;

#if WITH_EDITOR
	EDataValidationResult IsDataValid(FDataValidationContext& Context) const;
#endif

	bool InitializeRuntime(UE::Mass::FEntityBuilder& Builder);
	bool SaveDataConverter(FMassEntityManager& EntityManager, FInstancedStruct& SaveData);

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
struct Faerie::ItemData::TMassTypeTraits<FFaerieItemStorageFragment> : TMassTypeTraitsBase<FFaerieItemStorageFragment>
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

/*
 * Fragment to store sub-items inside an item.
 * Implemented as a SharedFragment to keep the storage object alive, as regular fragments are not memory-managed.
 * SaveData is exported as a FFaerieChildStackFragmentSaveData instance.
 */
USTRUCT()
struct FFaerieChildStackFragment : public FFaerieMassConstSharedFragment
{
	GENERATED_BODY()

	/*
	 * Children slots of an item. There can currently be up to five of these.
	 */
	UPROPERTY(EditInstanceOnly, Category = "ChildSlot")
	TArray<FFaerieInlineStackContainer> Slots;

#if WITH_EDITOR
	EDataValidationResult IsDataValid(FDataValidationContext& Context) const;
#endif

	bool InitializeRuntime(UE::Mass::FEntityBuilder& Builder);
	bool SaveDataConverter(FMassEntityManager& EntityManager, FInstancedStruct& SaveData);

____FAERIE_FRAGMENT_DECL(FFaerieChildStackFragment)
};

USTRUCT()
struct FFaerieChildStackFragmentSaveData : public FFaerieItemFragmentSaveData
{
	GENERATED_BODY()

	virtual const UScriptStruct* GetFragmentType() const override { return FFaerieChildStackFragment::StaticStruct(); }

	UPROPERTY()
	TArray<FInstancedStruct> Stacks;
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
struct Faerie::ItemData::TMassTypeTraits<FFaerieChildStackFragment> : TMassTypeTraitsBase<FFaerieChildStackFragment>
{
	enum
	{
		RequiresMutable = true
	};
};