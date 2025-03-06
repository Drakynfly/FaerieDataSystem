// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieAssetInfo.h"

#include "FaerieItemSource.generated.h"

enum class EFaerieItemMutabilityFlags : uint8;
class UFaerieItem;

UCLASS(Const)
class FAERIEITEMDATA_API UItemInstancingContext : public UObject
{
	GENERATED_BODY()

	friend class UFaerieItem;

public:
	// Flags to mark instances with
	UPROPERTY()
	EFaerieItemMutabilityFlags Flags;
};

// This class does not need to be modified.
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
	// Can this source create mutable items?
	virtual bool CanBeMutable() const { return false; }

	// Allows sources to give info about generation results
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemSource")
	virtual FFaerieAssetInfo GetSourceInfo() const { return FFaerieAssetInfo(); }

	// Create a item instance from this source.
	// An InstancingContext may be required to provide contextual data from the requester of the item.
	virtual UFaerieItem* CreateItemInstance(const UItemInstancingContext* Context) const
		PURE_VIRTUAL(IFaerieItemSource::CreateItemInstance, return nullptr; )
};

/**
 * A wrapper struct that can container a pointer to any object that implements IFaerieItemSource
 * Held reference is soft.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemSourceObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "/Script/FaerieItemData.FaerieItemSource"))
	TSoftObjectPtr<UObject> Object;

	friend bool operator==(const FFaerieItemSourceObject& Lhs, const FFaerieItemSourceObject& Rhs)
	{
		return Lhs.Object == Rhs.Object;
	}

	friend bool operator!=(const FFaerieItemSourceObject& Lhs, const FFaerieItemSourceObject& Rhs)
	{
		return !(Lhs == Rhs);
	}

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieItemSourceObject& Value)
	{
		return GetTypeHash(Value.Object);
	}
};