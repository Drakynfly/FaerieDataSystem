// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieFastArraySerializer.h"
#include "BinarySearchOptimizedArray.h"
#include "FaerieFastArraySerializerHack.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemKey.h"
#include "FaerieItemStackView.h"
#include "InventoryDataStructs.generated.h"

enum class EEntryEquivalencyFlags : uint8;

LLM_DECLARE_TAG(ItemStorage);

// Typesafe wrapper around an FFaerieItemKeyBase used for keying stacks in a UFaerieItemStorage.
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FStackKey : public FFaerieItemKeyBase
{
	GENERATED_BODY()
	using FFaerieItemKeyBase::FFaerieItemKeyBase;
};

USTRUCT()
struct FKeyedStack
{
	GENERATED_BODY()

	// Unique key to identify this stack.
	UPROPERTY(VisibleAnywhere, Category = "KeyedStack")
	FStackKey Key;

	// Amount in the stack
	UPROPERTY(VisibleAnywhere, Category = "KeyedStack")
	int32 Stack = 0;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FKeyedStack& Other) const
	{
		return Key == Other.Key && Stack == Other.Stack;
	}
};

struct FInventoryContent;
class UFaerieItem;

/**
 * The struct for containing one inventory entry.
 */
USTRUCT()
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FInventoryEntry() = default;
	FInventoryEntry(FFaerieItemStackView InStack, TArray<FFaerieAddress>& OutNewAddresses);

	// Unique key to identify this entry.
	UPROPERTY(VisibleAnywhere, Category = "InventoryEntry")
	FEntryKey Key;

private:
	// The item stored for all stacks in this entry.
	UPROPERTY(VisibleAnywhere, Category = "InventoryEntry")
	TObjectPtr<const UFaerieItem> ItemObject;

	UPROPERTY(VisibleAnywhere, Category = "InventoryEntry")
	TArray<FKeyedStack> Stacks;

	// Cached here for convenience, but this value is determined by UFaerieStackLimiterToken::GetItemStackLimit.
	int32 Limit = 0;

	// Internal count of how many stacks we've made. Used to track key creation. Only valid on the server.
	Faerie::TKeyGen<FStackKey> KeyGen;

	int32 GetStackIndex(FStackKey InKey) const;
	const FKeyedStack* GetStackPtr(FStackKey InKey) const;

	void UpdateCachedStackLimit();

public:
	UE_REWRITE const UFaerieItem* GetItem() const { return ItemObject; }
	UE_REWRITE TConstArrayView<FKeyedStack> GetStacks() const { return Stacks; }

	UE_REWRITE int32 NumStacks() const { return Stacks.Num(); }

	UE_REWRITE int32 GetCachedStackLimit() const { return Limit; }

	bool Contains(FStackKey Key) const;

	int32 GetStack(FStackKey Key) const;

	FStackKey GetStackAt(int32 Index) const;

	void CopyKeys(TArray<FStackKey>& OutKeys) const;
	void CopyAddresses(TArray<FFaerieAddress>& OutAddresses) const;
	void CopyStacks(TArray<int32>& OutStacks) const;

	int32 StackSum() const;

	bool IsValid() const;

	// Gets a view of the item and stack
	FFaerieItemStackView ToItemStackView() const;

	void PostSerialize(const FArchive& Ar);
	void PostScriptConstruct();

	void PreReplicatedRemove(const FInventoryContent& InArraySerializer);
	void PostReplicatedAdd(const FInventoryContent& InArraySerializer);
	void PostReplicatedChange(const FInventoryContent& InArraySerializer);

	static bool IsEqualTo(const FInventoryEntry& A, const FInventoryEntry& B, EEntryEquivalencyFlags CheckFlags);

	struct FMutableAccess : FNoncopyable
	{
		FMutableAccess(FInventoryContent& Source, const FInventoryEntry& Entry);
		FMutableAccess(FInventoryContent& Source, int32 Index);
		FMutableAccess(FInventoryContent& Source, const FEntryKey Key);
		~FMutableAccess();

		FInventoryEntry* operator->() const { return &Handle; }
		FInventoryEntry& Get() const { return Handle; }

		// Sets the number of copies in a stack. If stack is less that or equal to zero, will defer to RemoveStack.
		void SetStack(FStackKey InKey, const int32 Stack);

		// Removes a stack.
		void RemoveStack(FStackKey InKey);

		// Add the Amount to the stacks, adding new stacks as needed.
		// ReturnValue is 0 if Amount was successfully added, or the remainder, otherwise.
		void AddToAnyStack(int32 Amount, TArray<FFaerieAddress>& OutNewAddresses);

		// Add the Amount as new stacks.
		// ReturnValue is 0 if Amount was successfully added, or the remainder, otherwise.
		void AddToNewStacks(int32 Amount, TArray<FFaerieAddress>& OutNewAddresses);

		// Remove the amount from any number of stacks.
		// ReturnValue is 0 if Amount was successfully removed, or the remainder, if not.
		int32 RemoveFromAnyStack(int32 Amount, TArray<FFaerieAddress>& OutAllModifiedAddresses);

		// Move an amount from one stack to another.
		// ReturnValue is 0 if Amount was successfully moved, or the remainder, otherwise.
		int32 MoveStack(FStackKey From, FStackKey To, int32 Amount);

		// Split a stack into two. Returns the new stack key made.
		FStackKey SplitStack(FStackKey InKey, int32 Amount);

		// Utility to check if this entry contains only the given stack.
		bool IsOnlyStack(FStackKey InKey) const;

	protected:
		void MarkStackDirty(int32 Index);
		void MarkAllStacksDirty();

		FInventoryEntry& Handle;

		FKeyedStack* GetStackPtr(FStackKey InKey);

	private:
		FInventoryContent& Source;

		// Tracks the stacks that were changed (either added or had their value edited)
		TBitArray<> ChangeMask;
	};

	FMutableAccess GetMutableAccess(FInventoryContent& EntryMap) const
	{
		return FMutableAccess(EntryMap, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FInventoryEntry> : public TStructOpsTypeTraitsBase2<FInventoryEntry>
{
	enum
	{
		WithPostSerialize = true,
		WithPostScriptConstruct = true,
	};
};

class UFaerieItemStorage;

/**
 * FInventoryContent is a Fast Array, containing all FInventoryEntries for an inventory. Lookup is O(Log(n)), as FEntryKeys
 * are used to keep Entries in numeric order, allowing for binary-search accelerated accessors.
 */
USTRUCT()
struct FInventoryContent : public FFaerieFastArraySerializer
#if CPP
                           , public TBinarySearchOptimizedArray<FInventoryContent, FInventoryEntry>
#endif
{
	GENERATED_BODY()

	friend FInventoryEntry::FMutableAccess;
	friend TBinarySearchOptimizedArray;
	friend UFaerieItemStorage;

private:
	UPROPERTY(VisibleAnywhere, Category = "InventoryContent")
	TArray<FInventoryEntry> Entries;

	// Enables TBinarySearchOptimizedArray
	UE_REWRITE TArray<FInventoryEntry>& GetArray() { return Entries; }

	/** Owning storage to send Fast Array callbacks to */
	// UPROPERTY() Fast Arrays cannot have additional properties with Iris
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<UFaerieItemStorage> ChangeListener;

	// Is writing to Entries locked? Enabled while ItemHandles are active.
	mutable uint32 WriteLock = 0;

public:
	/**
	 * Adds a new key and entry to the end of the Items array. Performs a quick check that the new key is sequentially
	 * following the end of the array, but does not enforce or check for the entire array being sorted. Use this function
	 * when you can confirm that the key is sequential. When this is not known, use Insert instead. Append is O(1), while
	 * Insert is O(Log(n)), so use this if you can.
	 * */
	void Append(const FInventoryEntry& Entry);

	/**
	 * Works like Append, but doesn't check that the key is sequential. Use this when adding multiple items in quick
	 * succession, and you don't need the array sorted in the meantime. Sort must be called when you are done, to bring
	 * everything back into shape.
	 */
	void AppendUnsafe(const FInventoryEntry& Entry);

	/**
	 * Performs a binary search to find where to insert this new key. Needed when Key is not guaranteed to be sequential.
	 * @see Append
	 */
	void Insert(const FInventoryEntry& Entry);

	void Remove(FEntryKey Key);

	UE_REWRITE bool IsEmpty() const { return Entries.IsEmpty(); }

	UE_REWRITE int32 Num() const { return Entries.Num(); }

	// Low-level access to the WriteLock. Used to prevent added/removing data while iterating.
	void LockWriteAccess() const;
	void UnlockWriteAccess() const;
	UE_REWRITE UFaerieItemStorage* GetOuterItemStorage() const { return ChangeListener; }

	UE_REWRITE FInventoryEntry::FMutableAccess GetMutableEntry(const FEntryKey Key)
	{
		return FInventoryEntry::FMutableAccess(*this, Key);
	}

	UE_REWRITE bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return Faerie::Hacks::FastArrayDeltaSerialize<FInventoryEntry, FInventoryContent>(Entries, DeltaParms, *this);
	}

	enum EChangeType
	{
		// The server closed an edit handle
		Server_ItemHandleClosed,

		// A token in the item mutated
		ItemMutated,

		// The client has received a replication update
		Client_SomethingReplicated,
	};

	/*
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize) const;
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) const;
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) const;
	*/

	void PreEntryReplicatedRemove(const FInventoryEntry& Entry) const;
	void PostEntryReplicatedAdd(const FInventoryEntry& Entry) const;
	void PostEntryReplicatedChange_Server(const FInventoryEntry& Entry, EChangeType ChangeType, const TBitArray<>& ChangeMask) const;
	void PostEntryReplicatedChange_Client(const FInventoryEntry& Entry) const;

	// Only const iteration is allowed.
	using TRangedForConstIterator = TArray<FInventoryEntry>::RangedForConstIteratorType;
	TRangedForConstIterator begin() const;
	TRangedForConstIterator end() const;
};

template <>
struct TStructOpsTypeTraits<FInventoryContent> : public TStructOpsTypeTraitsBase2<FInventoryContent>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};