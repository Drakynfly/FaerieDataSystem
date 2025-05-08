// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemKey.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemContainerStructs.generated.h"

class UFaerieItemContainerBase;

// Typesafe wrapper around an FFaerieItemKeyBase used for keying entries in a UFaerieItemContainerBase.
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FEntryKey : public FFaerieItemKeyBase
{
	GENERATED_BODY()
	using FFaerieItemKeyBase::FFaerieItemKeyBase;

	static FEntryKey InvalidKey;
};

USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieAddress
{
	GENERATED_BODY()

	UPROPERTY()
	int64 Address;

	FORCEINLINE bool IsValid() const
	{
		return Address != 0;
	}

	friend bool operator==(const FFaerieAddress& Lhs, const FFaerieAddress& Rhs) { return Lhs.Address == Rhs.Address; }
	friend bool operator!=(const FFaerieAddress& Lhs, const FFaerieAddress& Rhs) { return !(Lhs == Rhs); }

	// Comparison operator for sorting, when this type is used as a Key.
	friend bool operator<(const FFaerieAddress& Lhs, const FFaerieAddress& Rhs)
	{
		return Lhs.Address < Rhs.Address;
	}

	friend FArchive& operator<<(FArchive& Ar, FFaerieAddress& Val)
	{
		return Ar << Val.Address;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieAddress& FaerieAddress)
	{
		return GetTypeHash(FaerieAddress.Address);
	}
};

/**
 * An item container and an address for some content.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieAddressableHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AddressableHandle")
	TWeakObjectPtr<UFaerieItemContainerBase> Container;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AddressableHandle")
	FFaerieAddress Address;

	bool IsValid() const;
};

/**
 * An item container and a key to an entry inside.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FContainerEntryHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InventoryKeyHandle")
	TWeakObjectPtr<UFaerieItemContainerBase> Container;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InventoryKeyHandle")
	FEntryKey Key;
};

/**
 * Struct to hold the data to save/load an inventory state from.
 */
USTRUCT()
struct FAERIEINVENTORY_API FFaerieContainerSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FInstancedStruct ItemData;

	UPROPERTY(SaveGame)
	TMap<FGuid, FInstancedStruct> ExtensionData;
};