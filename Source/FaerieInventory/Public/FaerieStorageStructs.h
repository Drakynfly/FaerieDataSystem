// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "DebuggingFlags.h"
#include "FaerieFastArraySerializer.h"
#include "BinarySearchOptimizedArray.h"
#include "FaerieFastArraySerializerHack.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemKey.h"
#include "FaerieStorageStructs.generated.h"

// Typesafe wrapper around an FFaerieItemKeyBase used for keying stacks in a UFaerieItemStorage.
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieStackKey : public FFaerieItemKeyBase
{
	GENERATED_BODY()
	using FFaerieItemKeyBase::FFaerieItemKeyBase;
};

USTRUCT()
struct FFaerieKeyedStack
{
	GENERATED_BODY()

	// Unique key to identify this stack.
	UPROPERTY(VisibleAnywhere, Category = "KeyedStack")
	FFaerieStackKey Key;

	// Amount in the stack
	UPROPERTY(VisibleAnywhere, Category = "KeyedStack")
	int32 Stack = 0;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieKeyedStack& Other) const
	{
		return Key == Other.Key && Stack == Other.Stack;
	}
};

struct FFaerieStorageContent;
class UFaerieItem;

/**
 * The struct for containing one inventory entry.
 */
USTRUCT()
struct FFaerieStorageEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	friend FFaerieStorageContent;
	friend TBinarySearchOptimizedArray;
	friend Faerie::Hacks::TFaerieFastArraySerializeHelper;

	FFaerieStorageEntry() = default;
	FFaerieStorageEntry(FFaerieEntryKey EntryKey, FFaerieItemInstance& Item, int32 StackLimit, int32 Amount, TArray<FFaerieAddress>& OutNewAddresses);
	FFaerieStorageEntry(FFaerieEntryKey EntryKey, FFaerieItemInstance& Item, int32 StackLimit, TConstArrayView<FFaerieKeyedStack> Stacks);

private:
	// Unique key to identify this entry.
	UPROPERTY(VisibleAnywhere, Category = "StorageEntry")
	FFaerieEntryKey Key;

	// The item stored for all stacks in this entry.
	UPROPERTY(VisibleAnywhere, Category = "StorageEntry")
	FFaerieItemInstance ItemInstance;

	UPROPERTY(VisibleAnywhere, Category = "StorageEntry")
	TArray<FFaerieKeyedStack> Stacks;

	// Cached here for convenience, but this value is determined by Faerie::Container::GetItemStackLimit.
	UPROPERTY(VisibleAnywhere, Category = "StorageEntry")
	int32 CachedLimit = 0;

	// Internal count of how many stacks we've made. Used to track key creation. Only valid on the server.
	Faerie::Inventory::TKeyGen<FFaerieStackKey> KeyGen;

	int32 GetStackIndex(FFaerieStackKey InKey) const;
	const FFaerieKeyedStack* GetStackPtr(FFaerieStackKey InKey) const;

	struct FStorageContentAccess
	{
		static const IFaerieItemOwnerInterface* GetListener(const FFaerieStorageContent& Source);
#if FAERIE_DEBUG
		static uint32& GetWriteLock(const FFaerieStorageContent& Source);
#endif
	};

public:
	UE_REWRITE FFaerieEntryKey GetKey() const { return Key; }
	UE_REWRITE const FFaerieItemInstance& GetInstance() const { return ItemInstance; }
	UE_REWRITE TConstArrayView<FFaerieKeyedStack> GetStacks() const { return Stacks; }

	UE_REWRITE int32 NumStacks() const { return Stacks.Num(); }

	UE_REWRITE int32 GetCachedStackLimit() const { return CachedLimit; }

	// Update the cached limit value for stacking mutable items. This usually doesn't need to be called, unless when
	// changing or setting the value in an instance that already exists in storage. There is currently no plumbing for this.
	void UpdateCachedStackLimit(const FMassEntityManager& EntityManager);

	bool Contains(FFaerieStackKey Key) const;

	int32 GetStack(FFaerieStackKey Key) const;

	FFaerieStackKey GetStackAt(int32 Index) const;

	FFaerieAddress FirstAddress() const;
	void CopyKeys(TArray<FFaerieStackKey>& OutKeys) const;
	void CopyAddresses(TArray<FFaerieAddress>& OutAddresses) const;
	void CopyStacks(TArray<int32>& OutStacks) const;

	int32 StackSum() const;

	bool IsValid() const;

	// Utility to check if this entry contains only the given stack.
	bool IsOnlyStack(FFaerieStackKey InKey) const;

	void PostSerialize(const FArchive& Ar);

	void PreReplicatedRemove(const FFaerieStorageContent& InArraySerializer);
	void PostReplicatedAdd(const FFaerieStorageContent& InArraySerializer);
	void PostReplicatedChange(const FFaerieStorageContent& InArraySerializer);

	struct FReadAccess final : Faerie::Container::IEntryView
	{
		UE_NONCOPYABLE(FReadAccess)

		FReadAccess(const FFaerieStorageContent& Source, const FFaerieStorageEntry& Entry)
		  : Entry(Entry), Source(Source) {}

		//~ ItemData::IViewBase
		UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Entry.ItemInstance; }
		UE_REWRITE virtual int32 GetCopies() const override { return Entry.StackSum(); }
		virtual const IFaerieItemOwnerInterface* GetItemOwner() const override;
		//~ ItemData::IViewBase

		//~ Container::IEntryView
		UE_REWRITE virtual FFaerieEntryKey ResolveKey() const override { return Entry.Key; }
		//~ Container::IEntryView

		UE_REWRITE const FFaerieStorageEntry* operator->() const { return &Entry; }
		UE_REWRITE const FFaerieStorageEntry& Get() const { return Entry; }

	protected:
		const FFaerieStorageEntry& Entry;

	private:
		const FFaerieStorageContent& Source;
	};

	struct FStackReadAccess final : Faerie::Container::IAddressView
	{
		UE_NONCOPYABLE(FStackReadAccess)

		FStackReadAccess(const FFaerieStorageContent& Source, const FFaerieStorageEntry& Entry, const FFaerieKeyedStack& Stack)
		  : Entry(Entry), Stack(Stack), Source(Source) {}

		//~ ItemData::IViewBase
		UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Entry.ItemInstance; }
		UE_REWRITE virtual int32 GetCopies() const override { return Stack.Stack; }
		virtual const IFaerieItemOwnerInterface* GetItemOwner() const override;
		//~ ItemData::IViewBase

		//~ Container::IEntryView
		UE_REWRITE virtual FFaerieEntryKey ResolveKey() const override { return Entry.Key; }
		//~ Container::IEntryView

		//~ Container::IAddressView
		virtual FFaerieAddress ResolveAddress() const override;
		//~ Container::IAddressView

		UE_REWRITE const FFaerieStorageEntry* operator->() const { return &Entry; }
		UE_REWRITE const FFaerieStorageEntry& Get() const { return Entry; }

	protected:
		const FFaerieStorageEntry& Entry;
		const FFaerieKeyedStack& Stack;

	private:
		const FFaerieStorageContent& Source;
	};

	struct FReadWriteAccess
	{
		UE_NONCOPYABLE(FReadWriteAccess)

		FReadWriteAccess(FFaerieStorageContent& Source, const FFaerieStorageEntry& Entry);
		~FReadWriteAccess();

		FFaerieStorageEntry* operator->() const { return &Entry; }
		FFaerieStorageEntry& Get() const { return Entry; }

		// Sets the number of copies in a stack. If stack is less that or equal to zero, will defer to RemoveStack.
		void SetStack(FFaerieStackKey InKey, const int32 Stack);

		// Removes a stack.
		void RemoveStack(FFaerieStackKey InKey);

		// Add the Amount to the stacks, adding new stacks as needed.
		// ReturnValue is 0 if Amount was successfully added, or the remainder, otherwise.
		void AddToAnyStack(int32 Amount, TArray<FFaerieAddress>& OutAddressesTouched);

		// Add the Amount as new stacks.
		// ReturnValue is 0 if Amount was successfully added, or the remainder, otherwise.
		void AddToNewStacks(int32 Amount, TArray<FFaerieAddress>& OutNewAddresses);

		// Remove the amount from any number of stacks.
		// ReturnValue is 0 if Amount was successfully removed, or the remainder, if not.
		int32 RemoveFromAnyStack(int32 Amount, TArray<FFaerieAddress>& OutAllModifiedAddresses);

		// Move an amount from one stack to another.
		// ReturnValue is 0 if Amount was successfully moved, or the remainder, otherwise.
		int32 MoveStack(FFaerieStackKey From, FFaerieStackKey To, int32 Amount);

		// Split a stack into two. Returns the new stack key made.
		FFaerieStackKey SplitStack(FFaerieStackKey InKey, int32 Amount);

	protected:
		//void MarkStackDirty(int32 Index);
		//void MarkAllStacksDirty();

		FFaerieStorageEntry& Entry;

		FFaerieKeyedStack* GetStackPtr(FFaerieStackKey InKey) const;

	private:
		FFaerieStorageContent& Source;

		// Tracks the stacks that were changed (either added or had their value edited)
		//TBitArray<> ChangeMask; // Disabled as it's unused.
	};

	UE_REWRITE FReadAccess GetReadAccess(const FFaerieStorageContent& EntryMap) const
	{
		return FReadAccess(EntryMap, *this);
	}

	UE_REWRITE FStackReadAccess GetStackReadAccess(const FFaerieStorageContent& EntryMap, const FFaerieStackKey Stack) const
	{
		return FStackReadAccess(EntryMap, *this, *GetStackPtr(Stack));
	}

	UE_REWRITE FReadWriteAccess GetReadWriteAccess(FFaerieStorageContent& EntryMap) const
	{
		return FReadWriteAccess(EntryMap, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FFaerieStorageEntry> : public TStructOpsTypeTraitsBase2<FFaerieStorageEntry>
{
	enum
	{
		WithPostSerialize = true,
	};
};

class UFaerieItemStorage;

/**
 * FFaerieStorageContent is a Fast Array, containing all FFaerieStorageEntries for an inventory. Lookup is O(Log(n)), as FFaerieEntryKeys
 * are used to keep Entries in numeric order, allowing for binary-search accelerated accessors.
 */
USTRUCT()
struct FFaerieStorageContent : public FFaerieFastArraySerializer
#if CPP
							   , public TBinarySearchOptimizedArray<FFaerieStorageContent, FFaerieStorageEntry>
#endif
{
	GENERATED_BODY()

	friend FFaerieStorageEntry;
	friend FFaerieStorageEntry::FStorageContentAccess;
	friend TBinarySearchOptimizedArray;
	friend UFaerieItemStorage;

private:
	UPROPERTY(VisibleAnywhere, Category = "InventoryContent")
	TArray<FFaerieStorageEntry> Entries;

	// Enables TBinarySearchOptimizedArray
	UE_REWRITE TArray<FFaerieStorageEntry>& GetArray() { return Entries; }

	/** Owning storage to send Fast Array callbacks to */
	// UPROPERTY() Fast Arrays cannot have additional properties with Iris
	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<UFaerieItemStorage> ChangeListener;

#if FAERIE_DEBUG
	// Is writing to Entries locked? Enabled while ItemHandles are active.
	mutable uint32 WriteLock = 0;
#endif

public:
	/**
	 * Adds a new key and entry to the end of the Items array. Performs a quick check that the new key is sequentially
	 * following the end of the array, but does not enforce or check for the entire array being sorted. Use this function
	 * when you can confirm that the key is sequential. When this is not known, use Insert_GetRef instead. Append is O(1), while
	 * Insert_GetRef is O(Log(n)), so use this if you can.
	 * */
	void Append(const FFaerieStorageEntry& Entry);

	/**
	 * Works like Append, but doesn't check that the key is sequential. Use this when adding multiple items in quick
	 * succession, and you don't need the array sorted in the meantime. Sort must be called when you are done, to bring
	 * everything back into shape.
	 */
	void AppendUnsafe(const FFaerieStorageEntry& Entry);

	/**
	 * Performs a binary search to find where to insert this new key. Needed when Key is not guaranteed to be sequential.
	 * @see Append
	 */
	void Insert(const FFaerieStorageEntry& Entry);

	void Remove(FFaerieEntryKey Key);

	UE_REWRITE bool IsEmpty() const { return Entries.IsEmpty(); }

	UE_REWRITE int32 Num() const { return Entries.Num(); }

#if FAERIE_DEBUG
	// Low-level access to the WriteLock. Used to prevent added/removing data while iterating.
	void LockWriteAccess() const;
	void UnlockWriteAccess() const;
#endif

	UE_REWRITE UFaerieItemStorage* GetOuterItemStorage() const { return ChangeListener; }

	UE_REWRITE bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return Faerie::Hacks::FastArrayDeltaSerialize<FFaerieStorageEntry, FFaerieStorageContent>(Entries, DeltaParms, *this);
	}

	/*
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize) const;
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) const;
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) const;
	*/

	// Only const iteration is allowed.
	using TRangedForConstIterator = TArray<FFaerieStorageEntry>::RangedForConstIteratorType;
	TRangedForConstIterator begin() const;
	TRangedForConstIterator end() const;
};

template <>
struct TStructOpsTypeTraits<FFaerieStorageContent> : public TStructOpsTypeTraitsBase2<FFaerieStorageContent>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

USTRUCT()
struct FFaerieStorageEntryExportData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UFaerieItem> ItemObject;

	UPROPERTY()
	FFaerieItemExportData ExportData;

	UPROPERTY()
	TArray<FFaerieKeyedStack> Stacks;
};


USTRUCT()
struct FFaerieStorageExportData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFaerieStorageEntryExportData> Entries;
};
