// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryItemLimitExtension.h"
#include "FaerieContainerIterator.h"
#include "FaerieInventoryContentLog.h"

#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryItemLimitExtension)

void UInventoryItemLimitExtension::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	for (auto It = Faerie::Container::KeyRange(Container); It; ++It)
	{
		UpdateCacheForEntry(It.GetKey(), It.GetCopies());
	}
}

void UInventoryItemLimitExtension::DeinitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	for (auto It = Faerie::Container::KeyRange(Container); It; ++It)
	{
		int32 Value = 0;
		EntryAmountCache.RemoveAndCopyValue(*It, Value);
		CurrentTotalItemCopies -= Value;
	}
}

EEventExtensionResponse UInventoryItemLimitExtension::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*>,
                                                                     const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
                                                                     const FFaerieExtensionAllowsAdditionArgs Args) const
{
	int32 TestCount = 0;

	switch (Args.TestType)
	{
	case EFaerieStorageAddStackTestMultiType::IndividualTests:
		{
			// Find the largest stack
			for (int32 i = 0; i < Proxies.Num(); ++i)
			{
				const FFaerieItemProxy Proxy = Proxies[i];
				TestCount = FMath::Max(TestCount, Proxy.GetCopies());
			}
		}
		break;
	case EFaerieStorageAddStackTestMultiType::GroupTest:
		{
			// Sum all stacks
			for (int32 i = 0; i < Proxies.Num(); ++i)
			{
				const FFaerieItemProxy Proxy = Proxies[i];
				TestCount += Proxy.GetCopies();
			}
		}
		break;
	}

	if (!CanContain(TestCount))
	{
		UE_LOGF(LogFaerieInventoryContent, VeryVerbose,
			"AllowsAddition: Cannot add Stack(s) (Total Count: %i)",
			TestCount);

		return EEventExtensionResponse::Disallowed;
	}

	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventoryItemLimitExtension::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events)
{
	for (auto&& Event : Events.Data)
	{
		if (Event.EntryRemoved)
		{
			// Entry was removed, delete cache.
			RemoveCacheForEntry(Event.EntryTouched);
		}
		else
		{
			if (Events.IsRemovalEvent())
			{
				// Event is removal, pass negative Copies as delta
				UpdateCacheForEntry(Event.EntryTouched, -Event.Copies);
			}
			else
			{
				UpdateCacheForEntry(Event.EntryTouched, Event.Copies);
			}
		}
	}
}

int32 UInventoryItemLimitExtension::GetTotalItemCount() const
{
	return CurrentTotalItemCopies;
}

int32 UInventoryItemLimitExtension::GetRemainingEntryCount() const
{
	if (MaxEntries <= 0)
	{
		return Faerie::ItemData::UnlimitedStack;
	}
	return MaxEntries - EntryAmountCache.Num();
}

int32 UInventoryItemLimitExtension::GetRemainingTotalItemCount() const
{
	if (MaxTotalItemCopies <= 0)
	{
		return Faerie::ItemData::UnlimitedStack;
	}
	return MaxTotalItemCopies - CurrentTotalItemCopies;
}

bool UInventoryItemLimitExtension::CanContain(const int32 Count) const
{
	if (MaxEntries > 0)
	{
		// Maximum entries reached check
		if (EntryAmountCache.Num() >= MaxEntries)
		{
			return false;
		}
	}

	if (MaxTotalItemCopies > 0)
	{
		// Maximum total item reached check
		if (CurrentTotalItemCopies + Count > MaxTotalItemCopies)
		{
			return false;
		}
	}

	return true;
}

void UInventoryItemLimitExtension::UpdateCacheForEntry(const FFaerieEntryKey Key, const int32 Delta)
{
	int32 EntryAmount = 0;
	if (auto&& ExistingCache = EntryAmountCache.Find(Key))
	{
		EntryAmount = *ExistingCache;
	}

	EntryAmount += Delta;
	check(EntryAmount > 0);

	EntryAmountCache.Add(Key, EntryAmount);
	CurrentTotalItemCopies += Delta;
}

void UInventoryItemLimitExtension::RemoveCacheForEntry(const FFaerieEntryKey Key)
{
	if (const int32* ExistingCache = EntryAmountCache.Find(Key))
	{
		CurrentTotalItemCopies -= *ExistingCache;
		EntryAmountCache.Remove(Key);
	}
}
