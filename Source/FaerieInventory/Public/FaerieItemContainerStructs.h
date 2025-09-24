// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemKey.h"
#include "FaerieItemContainerStructs.generated.h"

class UFaerieItemContainerBase;

// Typesafe wrapper around an FFaerieItemKeyBase used for keying entries in a UFaerieItemContainerBase.
// @todo address-refactor, remove API export of this type
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
	int64 Address = 0;

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

struct FFaerieItemProxy;

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

	FFaerieItemProxy ToProxy() const;
};