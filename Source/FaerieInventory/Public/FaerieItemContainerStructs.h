// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemKey.h"
#include "StructUtils/InstancedStruct.h"

#include "FaerieItemContainerStructs.generated.h"

// Typesafe wrapper around an FFaerieItemKeyBase used for keying entries in a UFaerieItemContainerBase.
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieEntryKey : public FFaerieItemKeyBase
{
	GENERATED_BODY()
	using FFaerieItemKeyBase::FFaerieItemKeyBase;

	static FFaerieEntryKey InvalidKey;
};

/*
 * A network stable key to refer to a stack of items in a faerie item container.
 * Usage is left to the implementation of the container class.
 */
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

struct FFaerieItemProxy;

/**
 * An alternative to FFaerieItemProxy that is stable to be sent over the network, but requires an additional level of
 * indirection, by not storing the proxy object, but rather, the Container Object, and a Faerie Address that can be used
 * as a lookup key.
 * In most cases FFaerieItemProxy should be used instead as it is faster to resolve.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieItemNetworkHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AddressableHandle")
	TWeakObjectPtr<UFaerieItemContainerBase> Container;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AddressableHandle")
	FFaerieAddress Address;

	bool IsValid() const;

	FFaerieItemProxy ResolveProxy() const;

	// Create a replicatable handle from a ItemProxy.
	static FFaerieItemNetworkHandle FromProxy(const FFaerieItemProxy& Proxy);

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FFaerieItemNetworkHandle> : public TStructOpsTypeTraitsBase2<FFaerieItemNetworkHandle>
{
	enum
	{
		WithNetSerializer = true,
	};
};

USTRUCT()
struct FAERIEINVENTORY_API FFaerieItemExportData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInstancedStruct> MassInstances;
};