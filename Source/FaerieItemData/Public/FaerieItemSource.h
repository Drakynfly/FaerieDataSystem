// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"

#include "UObject/Interface.h"
#include "FaerieUnownedItemStack.h"

#include "FaerieItemSource.generated.h"

struct FFaerieItemInstancingContext;

namespace Faerie::ItemData
{
	/*
	 * Utility to build item data via a generic API that works for both runtime item instances that live in Mass, and
	 * item asset editor data.
	 */
	USTRUCT()
	struct FAERIEITEMDATA_API FItemDataBuilder
#if CPP
		final : public IViewBase
#endif
	{
		GENERATED_BODY()

		FItemDataBuilder() = default;
		FItemDataBuilder(const FMassEntityHandle Item, const int32 Copies)
			: RuntimeItem(Item), Copies(Copies) {}
#if WITH_EDITOR
		FItemDataBuilder(UFaerieItemAsset* ItemAsset, const int32 Copies)
			: ItemAsset(ItemAsset), Copies(Copies) {}
#endif

		//~ IViewBase
#if WITH_EDITOR
		UE_REWRITE virtual const UFaerieItemAsset* GetItemAsset() const override { return ItemAsset; }
#endif
		UE_REWRITE virtual FMassEntityHandle GetEntityHandle() const override { return RuntimeItem; }
		UE_REWRITE virtual int32 GetCopies() const override { return Copies; }
		//~ IViewBase

	public:
		// Initialize the item and return a fully valid item instance.
		FFaerieUnownedItemStack GetForRuntime() const;

#if WITH_EDITOR
		// Extract the item stack without initializing the instance.
		UE_REWRITE TPair<UFaerieItemAsset*, int32> GetForEditor() const
		{
			return {ItemAsset, Copies};
		}
#endif

		bool IsValid() const;

		void SetCopies(const int32 InCopies)
		{
			Copies = InCopies;
		}

	private:
#if WITH_EDITORONLY_DATA
		UPROPERTY()
		TObjectPtr<UFaerieItemAsset> ItemAsset;
#endif

		FMassEntityHandle RuntimeItem;
		int32 Copies = 0;
	};
}

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class FAERIEITEMDATA_API UFaerieItemSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface added to classes that can author a faerie item instance
 */
class FAERIEITEMDATA_API IFaerieItemSource
{
	GENERATED_BODY()

public:
	// AssetData tag to add in GetAssetRegistryTags in implementing classes that generate mutable instances.
	static const FName MutableSourceTag;

	// Can this source create mutable items? If this can return true in an implementing class, you must also override
	// GetAssetRegistryTags to add MutableSourceTag.
	virtual bool CanBeMutable() const { return false; }

	// Create an item stack from this source.
	// An InstancingContext may be required to provide contextual data from the requester of the stack, depending on the implementation.
	virtual Faerie::ItemData::FItemDataBuilder CreateItemStack(const FFaerieItemInstancingContext& Context) const
		PURE_VIRTUAL(IFaerieItemSource::CreateItemStack, return Faerie::ItemData::FItemDataBuilder(); )
};

/**
 * A wrapper struct that contains a soft reference to an object implementing IFaerieItemSource
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemSourceObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "/Script/FaerieItemData.FaerieItemSource"))
	TSoftObjectPtr<const UObject> Object;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemSourceObject& Other) const
	{
		return Object == Other.Object;
	}

	[[nodiscard]] UE_REWRITE friend uint32 GetTypeHash(const FFaerieItemSourceObject& Value)
	{
		return GetTypeHash(Value.Object);
	}
};