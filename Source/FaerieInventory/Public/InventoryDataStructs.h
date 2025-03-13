// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieDefinitions.h"
#include "BinarySearchOptimizedArray.h"
#include "FaerieFastArraySerializerHack.h"
#include "FaerieItemStackView.h"
#include "GameplayTagContainer.h"
#include "TTypedTagStaticImpl2.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "StructUtils/StructView.h"
#include "InventoryDataStructs.generated.h"

enum class EEntryEquivalencyFlags : uint8;

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryStructs, Log, All)

LLM_DECLARE_TAG(ItemStorage);

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

	int32 Value() const { return KeyValue; }

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

// Typesafe wrapper around an FFaerieItemKeyBase used for keying entries.
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FEntryKey : public FFaerieItemKeyBase
{
	GENERATED_BODY()
	using FFaerieItemKeyBase::FFaerieItemKeyBase;

	static FEntryKey InvalidKey;
};

// Typesafe wrapper around an FFaerieItemKeyBase used for keying stacks.
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FStackKey : public FFaerieItemKeyBase
{
	GENERATED_BODY()
	using FFaerieItemKeyBase::FFaerieItemKeyBase;
};


USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FKeyedStack
{
	GENERATED_BODY()

	// Unique key to identify this stack.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "KeyedStack")
	FStackKey Key;

	// Amount in the stack
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "KeyedStack")
	int32 Stack = 0;

	friend bool operator==(const FKeyedStack& Lhs, const FStackKey Rhs)
	{
		return Lhs.Key == Rhs;
	}

	friend bool operator==(const FKeyedStack& Lhs, const FKeyedStack& Rhs)
	{
		return Lhs.Key == Rhs.Key
			&& Lhs.Stack == Rhs.Stack;
	}

	friend bool operator!=(const FKeyedStack& Lhs, const FKeyedStack& Rhs)
	{
		return !(Lhs == Rhs);
	}
};


/**
 * An entry key and stack key pair. This is a true "key" to an exact inventory stack as seen by an external interface.
 */
USTRUCT(BlueprintType)
struct FInventoryKey
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InventoryKey")
	FEntryKey EntryKey;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InventoryKey")
	FStackKey StackKey;

	FORCEINLINE friend uint32 GetTypeHash(const FInventoryKey Key)
	{
		return HashCombine(GetTypeHash(Key.EntryKey), GetTypeHash(Key.StackKey));
	}

	bool IsValid() const { return EntryKey.IsValid() && StackKey.IsValid(); }

	FString ToString() const
	{
		return EntryKey.ToString() + TEXT(":") + StackKey.ToString();
	}

	friend bool operator==(const FInventoryKey Lhs, const FInventoryKey Rhs)
	{
		return Lhs.EntryKey == Rhs.EntryKey
			&& Lhs.StackKey == Rhs.StackKey;
	}

	friend bool operator!=(const FInventoryKey Lhs, const FInventoryKey Rhs)
	{
		return !(Lhs == Rhs);
	}

	friend bool operator<(const FInventoryKey Lhs, const FInventoryKey Rhs)
	{
		if (Lhs.EntryKey == Rhs.EntryKey)
		{
			return Lhs.StackKey < Rhs.StackKey;
		}

		return Lhs.EntryKey < Rhs.EntryKey;
	}
};

/**
 * The key used to flag entries with custom data.
 */
USTRUCT(BlueprintType, meta = (Categories = "Fae.Inventory"))
struct FFaerieInventoryTag : public FGameplayTag
{
	GENERATED_BODY()
	END_TAG_DECL2(FFaerieInventoryTag, TEXT("Fae.Inventory"))
};

class UFaerieItem;

/**
 * The struct for containing one inventory entry.
*/
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FInventoryEntry
{
	GENERATED_BODY()

	// Actual data for this entry.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryEntry")
	TObjectPtr<UFaerieItem> ItemObject;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryEntry")
	TArray<FKeyedStack> Stacks;

	// Cached here for convenience, but this value is determined by UFaerieStackLimiterToken::GetItemStackLimit.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InventoryEntry")
	int32 Limit = 0;

private:
	// Internal count of how many stacks we've made. Used to track key creation. Only valid on the server.
	Faerie::TKeyGen<FStackKey> KeyGen;

	int32 GetStackIndex(FStackKey Key) const;
	FKeyedStack* GetStackPtr(FStackKey Key);
	const FKeyedStack* GetStackPtr(FStackKey Key) const;

public:
	bool Contains(FStackKey Key) const;

	int32 GetStack(FStackKey Key) const;

	TArray<FStackKey> CopyKeys() const;

	int32 StackSum() const;

	void SetStack(FStackKey, const int32 Stack);

	// Add the Amount to the stacks, adding new stacks as needed. Can optionally return the list of added stacks.
	// ReturnValue is 0 if Stack was successfully added, or the remainder, otherwise.
	void AddToAnyStack(int32 Amount, TArray<FStackKey>* OutAddedKeys = nullptr);

	// Add the Amount as new stacks. Can optionally return the list of added stacks.
	// ReturnValue is 0 if Stack was successfully added, or the remainder, otherwise.
	void AddToNewStacks(int32 Amount, TArray<FStackKey>* OutAddedKeys = nullptr);

	// Remove the amount from any number of stacks. Can optionally return the list of modified stacks, and/or just the removed stacks
	// ReturnValue is 0 if Stack was successfully removed, or the remainder, if not.
	int32 RemoveFromAnyStack(int32 Amount, TArray<FStackKey>* OutAllModifiedKeys = nullptr, TArray<FStackKey>* OutRemovedKeys = nullptr);

	// Move an amount from one stack to another.
	// The amount not moved will be returned, if some remains.
	int32 MoveStack(FStackKey From, FStackKey To, int32 Amount);

	// Split a stack into two. Returns the new stack key made.
	FStackKey SplitStack(FStackKey Key, int32 Amount);

	bool IsValid() const;

	// Gets a view of the item and stack
	FFaerieItemStackView ToItemStackView() const;

	void PostSerialize(const FArchive& Ar);

	static bool IsEqualTo(const FInventoryEntry& A, const FInventoryEntry& B, EEntryEquivalencyFlags CheckFlags);
};

template<>
struct TStructOpsTypeTraits<FInventoryEntry> : public TStructOpsTypeTraitsBase2<FInventoryEntry>
{
	enum
	{
		WithPostSerialize = true,
	};
};

using FInventoryEntryView = TConstStructView<FInventoryEntry>;

struct FInventoryContent;

// A minimal struct to replicate a Key and Value pair as an emulation of a TMap
USTRUCT()
struct FKeyedInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FKeyedInventoryEntry() = default;

	FKeyedInventoryEntry(const FEntryKey Key, const FInventoryEntry& Value)
	  : Key(Key),
		Value(Value) {}

	// Unique key to identify this entry.
	UPROPERTY(VisibleAnywhere, Category = "KeyedInventoryEntry")
	FEntryKey Key;

	// A canonical entry.
	UPROPERTY(VisibleAnywhere, Category = "KeyedInventoryEntry")
	FInventoryEntry Value;

	void PreReplicatedRemove(const FInventoryContent& InArraySerializer);
	void PostReplicatedAdd(const FInventoryContent& InArraySerializer);
	void PostReplicatedChange(const FInventoryContent& InArraySerializer);
};

/**
 * FInventoryContent is a Fast Array, containing all FInventoryEntries for an inventory. Lookup is O(Log(n)), as FEntryKeys
 * are used to keep Entries in numeric order, allowing for binary-search accelerated accessors.
 */
USTRUCT()
struct FInventoryContent : public FFaerieFastArraySerializer,
                           public TBinarySearchOptimizedArray<FInventoryContent, FKeyedInventoryEntry>
{
	GENERATED_BODY()

	friend TBinarySearchOptimizedArray;
	friend class UFaerieItemStorage;

private:
	UPROPERTY(VisibleAnywhere, Category = "InventoryContent")
	TArray<FKeyedInventoryEntry> Entries;

	// Enables TBinarySearchOptimizedArray
	TArray<FKeyedInventoryEntry>& GetArray() { return Entries; }

	/** Owning storage to send Fast Array callbacks to */
	UPROPERTY()
	TWeakObjectPtr<UFaerieItemStorage> ChangeListener;

public:
	/**
	 * Adds a new key and entry to the end of the Items array. Performs a quick check that the new key is sequentially
	 * following the end of the array, but does not enforce or check for the entire array being sorted. Use this function
	 * when you can confirm that the key is sequential. When this is not known, use Insert instead. Append is O(1), while
	 * Insert is O(Log(n)), so use this if you can.
	 * */
	FKeyedInventoryEntry& Append(FEntryKey Key, const FInventoryEntry& Entry);

	/**
	 * Works like Append, but doesn't check that the key is sequential. Use this when adding multiple items in quick
	 * succession, and you don't need the array sorted in the meantime. Sort must be called when you are done, to bring
	 * everything back into shape.
	 */
	FKeyedInventoryEntry& AppendUnsafe(FEntryKey Key, const FInventoryEntry& Entry);

	/**
	 * Performs a binary search to find where to insert this new key. Needed when Key is not guaranteed to be sequential.
	 * @see Append
	 */
	void Insert(FEntryKey Key, const FInventoryEntry& Entry);

	void Remove(FEntryKey Key);

	bool IsEmpty() const { return Entries.IsEmpty(); }

	int32 Num() const { return Entries.Num(); }

	struct FScopedItemHandle : FNoncopyable
	{
		FScopedItemHandle(const FEntryKey Key, FInventoryContent& Source)
		  : Handle(Source.Entries[Source.IndexOf(Key)]),
			Source(Source) {}

		~FScopedItemHandle();

	protected:
		FKeyedInventoryEntry& Handle;

	private:
		FInventoryContent& Source;

	public:
		FInventoryEntry* operator->() const { return &Handle.Value; }
		FInventoryEntry& Get() const { return Handle.Value; }
	};

	FScopedItemHandle GetHandle(const FEntryKey Key)
	{
		return FScopedItemHandle(Key, *this);
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return Faerie::Hacks::FastArrayDeltaSerialize<FKeyedInventoryEntry, FInventoryContent>(Entries, DeltaParms, *this);
	}

	/*
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize) const;
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) const;
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) const;
	*/

	void PreEntryReplicatedRemove(const FKeyedInventoryEntry& Entry) const;
	void PostEntryReplicatedAdd(const FKeyedInventoryEntry& Entry) const;
	void PostEntryReplicatedChange(const FKeyedInventoryEntry& Entry) const;

	// Only const iteration is allowed.
	using TRangedForConstIterator = TArray<FKeyedInventoryEntry>::RangedForConstIteratorType;
	FORCEINLINE TRangedForConstIterator begin() const { return TRangedForConstIterator(Entries.begin()); }
	FORCEINLINE TRangedForConstIterator end() const { return TRangedForConstIterator(Entries.end()); }
};

template <>
struct TStructOpsTypeTraits<FInventoryContent> : public TStructOpsTypeTraitsBase2<FInventoryContent>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * An item storage object and a key to an element inside.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FInventoryKeyHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InventoryKeyHandle")
	TWeakObjectPtr<UFaerieItemStorage> ItemStorage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InventoryKeyHandle")
	FInventoryKey Key;
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