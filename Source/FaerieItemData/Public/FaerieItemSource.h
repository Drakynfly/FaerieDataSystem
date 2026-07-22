// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "FaerieUnownedItemStack.h"

#include "FaerieItemSource.generated.h"

struct FFaerieItemInstancingContext;

namespace Faerie::ItemData
{
	struct FAERIEITEMDATA_API FGetInstanceResult
	{
		UE_NONCOPYABLE(FGetInstanceResult)
		FGetInstanceResult(const TOptional<FFaerieUnownedItemStack>& Stack)
		  : Stack(Stack) {}
		FGetInstanceResult(FNullOpt) : Stack(NullOpt) {}

		// Initialize the item and return a fully valid item instance.
		FFaerieUnownedItemStack WithInitialization() const;

		// Extract the item stack without initializing the instance.
		UE_REWRITE FFaerieUnownedItemStack WithoutInitialization() const
		{
			return Stack.GetValue();
		}

		UE_REWRITE bool IsValid() const
		{
			return Stack.IsSet();
		}

	private:
		TOptional<FFaerieUnownedItemStack> Stack;
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
	// GetAssetRegistryTags and add MutableSourceTag.
	virtual bool CanBeMutable() const { return false; }

	// Create an item stack from this source.
	// An InstancingContext may be required to provide contextual data from the requester of the stack, depending on the implementation.
	virtual Faerie::ItemData::FGetInstanceResult CreateItemStack(const FFaerieItemInstancingContext& Context) const
		PURE_VIRTUAL(IFaerieItemSource::CreateItemStack, return NullOpt; )
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

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemSourceObject& Value)
	{
		return GetTypeHash(Value.Object);
	}
};