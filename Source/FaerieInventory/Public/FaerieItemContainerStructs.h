// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemKey.h"
#include "FaerieItemContainerStructs.generated.h"

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
	int64 Address = 0;

	UE_REWRITE bool IsValid() const
	{
		return Address != 0;
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieAddress& Other) const
	{
		return Address == Other.Address;
	}

	// Comparison operator for sorting, when this type is used as a Key.
	[[nodiscard]] UE_REWRITE bool UEOpLessThan(const FFaerieAddress& Other) const
	{
		return Address < Other.Address;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieAddress& Value)
	{
		return GetTypeHash(Value.Address);
	}

	friend FArchive& operator<<(FArchive& Ar, FFaerieAddress& Value)
	{
		return Ar << Value.Address;
	}
};

class UFaerieItemContainerBase;

/**
 * An item container and a key for some content.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieEntryHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AddressableHandle")
	TWeakObjectPtr<UFaerieItemContainerBase> Container;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AddressableHandle")
	FEntryKey Key;

	bool IsValid() const;
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

