// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryItemLimitExtension.h"

#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryItemLimitExtension)

void UInventoryItemLimitExtension::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;

	Container->ForEachKey(
		[this, Container](const FEntryKey Key)
		{
			UpdateCacheForEntry(Container, Key);
		});
}

void UInventoryItemLimitExtension::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;

	Container->ForEachKey(
		[this](const FEntryKey Key)
		{
			int32 Value = 0;
			EntryAmountCache.RemoveAndCopyValue(Key, Value);
			CurrentTotalItemCopies -= Value;
		});
}

EEventExtensionResponse UInventoryItemLimitExtension::AllowsAddition(const UFaerieItemContainerBase*,
                                                                     const FFaerieItemStackView Stack,
                                                                     const EFaerieStorageAddStackBehavior) const
{
	if (!CanContain(Stack))
	{
		// @todo this is a routine call, which is normal to fail. Why log failure at all?
		UE_LOG(LogTemp, VeryVerbose,
			TEXT("AllowsAddition: Cannot add Stack (Copies: %i)"),
			Stack.Copies);

		return EEventExtensionResponse::Disallowed;
	}

	return EEventExtensionResponse::Allowed;
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

bool UInventoryItemLimitExtension::CanContain(const FFaerieItemStackView Stack) const
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
		if (CurrentTotalItemCopies + Stack.Copies > MaxTotalItemCopies)
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