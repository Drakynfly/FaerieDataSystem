﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryItemLimitExtension.h"
#include "FaerieInventoryContentLog.h"

#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryItemLimitExtension)

void UInventoryItemLimitExtension::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;

	for (const FEntryKey Key : Faerie::KeyRange(Container))
	{
		UpdateCacheForEntry(Container, Key);
	}
}

void UInventoryItemLimitExtension::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;

	for (const FEntryKey Key : Faerie::KeyRange(Container))
	{
		int32 Value = 0;
		EntryAmountCache.RemoveAndCopyValue(Key, Value);
		CurrentTotalItemCopies -= Value;
	}
}

EEventExtensionResponse UInventoryItemLimitExtension::AllowsAddition(const UFaerieItemContainerBase*,
                                                                     const TConstArrayView<FFaerieItemStackView> Views,
                                                                     const FFaerieExtensionAllowsAdditionArgs Args) const
{
	int32 TestCount = 0;

	switch (Args.TestType)
	{
	case EFaerieStorageAddStackTestMultiType::IndividualTests:
		{
			// Find the largest stack
			for (auto&& View : Views)
			{
				TestCount = FMath::Max(TestCount, View.Copies);
			}
		}
		break;
	case EFaerieStorageAddStackTestMultiType::GroupTest:
		{
			// Sum all stacks
			for (auto&& View : Views)
			{
				TestCount += View.Copies;
			}
		}
		break;
	}

	if (!CanContain(TestCount))
	{
		UE_LOG(LogFaerieInventoryContent, VeryVerbose,
			TEXT("AllowsAddition: Cannot add Stack(s) (Total Count: %i)"),
			TestCount);

		return EEventExtensionResponse::Disallowed;
	}

	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventoryItemLimitExtension::PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	UpdateCacheForEntry(Container, Event.EntryTouched);
}

void UInventoryItemLimitExtension::PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	UpdateCacheForEntry(Container, Event.EntryTouched);
}

void UInventoryItemLimitExtension::PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	UpdateCacheForEntry(Container, Event.EntryTouched);
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

void UInventoryItemLimitExtension::UpdateCacheForEntry(const UFaerieItemContainerBase* Container, const FEntryKey Key)
{
	if (!ensure(IsValid(Container))) return;

	int32 PrevEntryAmount = 0;
	if (auto&& ExistingCache = EntryAmountCache.Find(Key))
	{
		PrevEntryAmount = *ExistingCache;
	}

	if (!Container->Contains(Key))
	{
		CurrentTotalItemCopies -= PrevEntryAmount;
		EntryAmountCache.Remove(Key);
		return;
	}

	const int32 StackAtKey = Container->GetStack(Key);
	const int32 Diff = StackAtKey - PrevEntryAmount;

	EntryAmountCache.Add(Key, StackAtKey);
	CurrentTotalItemCopies += Diff;
}