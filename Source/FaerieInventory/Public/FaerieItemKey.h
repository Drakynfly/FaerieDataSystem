// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemKey.generated.h"

/**
 * A unique key that maps to a faerie item in some way that is persistent across the network and play-sessions.
 * Usually only guaranteed to be unique per implementing container.
 * Moving an item between containers will not preserve the key.
 */
USTRUCT(BlueprintType)
struct FFaerieItemKeyBase
{
	GENERATED_BODY()

	FFaerieItemKeyBase() = default;

	explicit FFaerieItemKeyBase(const int32 Value)
	  : KeyValue(Value) {}

private:
	UPROPERTY(VisibleAnywhere, Category = "FaerieItemKeyBase")
	int32 KeyValue = INDEX_NONE;

public:
	bool IsValid() const
	{
		return KeyValue > INDEX_NONE;
	}

	friend bool operator==(const FFaerieItemKeyBase& Lhs, const FFaerieItemKeyBase& Rhs)
	{
		return Lhs.KeyValue == Rhs.KeyValue;
	}

	friend bool operator!=(const FFaerieItemKeyBase& Lhs, const FFaerieItemKeyBase& Rhs)
	{
		return !(Lhs == Rhs);
	}

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieItemKeyBase& Key)
	{
		return Key.KeyValue;
	}

	friend bool operator<(const FFaerieItemKeyBase& Lhs, const FFaerieItemKeyBase& Rhs)
	{
		return Lhs.KeyValue < Rhs.KeyValue;
	}

	friend bool operator<=(const FFaerieItemKeyBase& Lhs, const FFaerieItemKeyBase& Rhs) { return Rhs >= Lhs; }
	friend bool operator>(const FFaerieItemKeyBase& Lhs, const FFaerieItemKeyBase& Rhs) { return Rhs < Lhs; }
	friend bool operator>=(const FFaerieItemKeyBase& Lhs, const FFaerieItemKeyBase& Rhs) { return !(Lhs < Rhs); }

	FORCEINLINE int32 Value() const { return KeyValue; }

	/** Get internal value as string for debugging */
	FString ToString() const { return FString::FromInt(KeyValue); }

	friend FArchive& operator<<(FArchive& Ar, FFaerieItemKeyBase& Val)
	{
		return Ar << Val.KeyValue;
	}
};

namespace Faerie
{
	// Create a new key from each integer in order. Guarantees unique keys are generated in a binary searchable order.
	template <
		typename TKey
		UE_REQUIRES(TIsDerivedFrom<TKey, FFaerieItemKeyBase>::Value)
	>
	class TKeyGen
	{
	public:
		// Creates the next unique key for an entry.
		TKey NextKey()
		{
			return TKey(++PreviousKey);
		}

		void SetPosition(const TKey Key)
		{
			ensureMsgf(Key.Value() > PreviousKey, TEXT("SetPosition should not be called, if it reversed to key order. In case of a full reset, call Reset first!"));
			PreviousKey = Key.Value();
		}

		void Reset()
		{
			PreviousKey = 100;
		}

	private:
		int32 PreviousKey = 100;
	};
}