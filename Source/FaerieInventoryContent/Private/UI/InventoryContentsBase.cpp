// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryContentsBase.h"
#include "FaerieInventoryContentLog.h"
#include "UI/InventoryFillMeterBase.h"
#include "UI/InventoryUIAction.h"

#include "FaerieItemDataComparator.h"
#include "FaerieItemDataFilter.h"

#include "Components/PanelWidget.h"
#include "UI/InventoryUIActionContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryContentsBase)

#define LOCTEXT_NAMESPACE "InventoryContentsBase"

UInventoryContentsBase::UInventoryContentsBase(const FObjectInitializer& ObjectInitializer)
  : Super(ObjectInitializer)
{
	ActionContainer = CreateDefaultSubobject<UInventoryUIActionContainer>(TEXT("ActionContainer"));
}

bool UInventoryContentsBase::Initialize()
{
	Query.Filter.BindUObject(this, &ThisClass::ExecFilter);
	Query.Sort.BindUObject(this, &ThisClass::ExecSort);

	return Super::Initialize();
}

void UInventoryContentsBase::NativeConstruct()
{
	Super::NativeConstruct();

	// Resort and display items whenever we are reconstructed with an existing inventory.
	if (ItemStorage.IsValid())
	{
		InitWithInventory(ItemStorage.Get());
	}
}

void UInventoryContentsBase::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (NeedsResort)
	{
		if (ItemStorage.IsValid())
		{
			ItemStorage->QueryAll(Query, SortedAndFilteredAddresses);
		}
		NeedsReconstructEntries = true;
		NeedsResort = false;
	}

	if (NeedsReconstructEntries)
	{
		DisplaySortedEntries();
		NeedsReconstructEntries = false;
	}
}

void UInventoryContentsBase::Reset()
{
	SortedAndFilteredAddresses.Empty();
	ActiveFilterRule = nullptr;
	ActiveSortRule = nullptr;
	Query.InvertFilter = false;
	Query.InvertSort = false;

	ResetFilter(false);
	ResetSort(false);

	if (ItemStorage.IsValid())
	{
		ItemStorage->GetOnAddressEvent().RemoveAll(this);
	}

	OnReset();

	ItemStorage = nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
// Cannot be const to bind
bool UInventoryContentsBase::ExecFilter(const FFaerieItemSnapshot& Entry)
{
	if (IsValid(ActiveFilterRule))
	{
		FFaerieItemStackView View;
		View.Item = Entry.ItemObject;
		View.Copies = Entry.Copies;
		return ActiveFilterRule->Exec(View);
	}
	return true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
// Cannot be const to bind
bool UInventoryContentsBase::ExecSort(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B)
{
	if (!IsValid(ActiveSortRule)) return false;
	return ActiveSortRule->Exec(A, B);
}

void UInventoryContentsBase::HandleAddressEvent(UFaerieItemStorage* Storage, const EFaerieAddressEventType Type, const FFaerieAddress Address)
{
	switch (Type)
	{
	case EFaerieAddressEventType::PostAdd:
		if (bAlwaysAddNewToSortOrder)
		{
			AddToSortOrder(Address, true);
			OnKeyAdded(Address);
		}
		break;
	case EFaerieAddressEventType::PreRemove:
		if (!!SortedAndFilteredAddresses.Remove(Address))
		{
			OnKeyRemoved(Address);
		}
		break;
	case EFaerieAddressEventType::Edit:
		if (bAlwaysAddNewToSortOrder)
		{
			AddToSortOrder(Address, false);
			OnKeyUpdated(Address);
		}
		break;
	}
}

void UInventoryContentsBase::SetLinkedStorage(UFaerieItemStorage* Storage)
{
	if (IsConstructed())
	{
		InitWithInventory(Storage);
	}
	else
	{
		ItemStorage = Storage;
	}
}

void UInventoryContentsBase::InitWithInventory(UFaerieItemStorage* Storage)
{
	// Reset state fully.
	Reset();

	if (IsValid(Storage))
	{
		ItemStorage = Storage;

		ItemStorage->GetOnAddressEvent().AddUObject(this, &ThisClass::HandleAddressEvent);

		OnInitWithInventory();

		// Load in entries that should be initially displayed
		NeedsResort = true;
	}
}

void UInventoryContentsBase::AddToSortOrder(const FFaerieAddress Address, const bool WarnIfAlreadyExists)
{
	struct FInsertKeyPredicate
	{
		FInsertKeyPredicate(const Faerie::FStorageQuery& Query, UFaerieItemStorage* Storage)
		  : Query(Query), Storage(Storage)
		{
			SnapA.Owner = Storage;
			SnapB.Owner = Storage;
		}

		const Faerie::FStorageQuery& Query;
		const UFaerieItemStorage* Storage;
		mutable FFaerieItemSnapshot SnapA, SnapB;

		bool operator()(const FFaerieAddress A, const FFaerieAddress B) const
		{
			const FFaerieItemStackView StorageA = Storage->ViewStack(A);
			const FFaerieItemStackView StorageB = Storage->ViewStack(B);
			SnapA.ItemObject = StorageA.Item.Get();
			SnapA.Copies = StorageA.Copies;
			SnapB.ItemObject = StorageB.Item.Get();
			SnapB.Copies = StorageB.Copies;

			const bool Result = Query.Sort.Execute(SnapA, SnapB);
			return Query.InvertSort ? !Result : Result;
		}
	};

	if (SortedAndFilteredAddresses.IsEmpty())
	{
		SortedAndFilteredAddresses.Add(Address);
	}
	else
	{
		if (!ActiveSortRule)
		{
			UE_LOG(LogFaerieInventoryContent, Verbose, TEXT("ActiveSortRule is invalid. Content will not be sorted!"));
			if (SortedAndFilteredAddresses.Find(Address) != INDEX_NONE)
			{
				if (WarnIfAlreadyExists)
				{
					UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Cannot add sort key that already exists in the array"));
				}
			}
			else
			{
				SortedAndFilteredAddresses.Add(Address);
				NeedsReconstructEntries = true;
			}
			return;
		}

		// Binary search to find position to insert the new key.
		const int32 Index = Algo::LowerBound(SortedAndFilteredAddresses, Address, FInsertKeyPredicate(Query, ItemStorage.Get()));

		// Return if the key we were sorted to or above is ourself.
		if ((SortedAndFilteredAddresses.IsValidIndex(Index) && (SortedAndFilteredAddresses[Index] == Address)) ||
			((SortedAndFilteredAddresses.IsValidIndex(Index+1) && (SortedAndFilteredAddresses[Index+1] == Address))))
		{
			if (WarnIfAlreadyExists)
			{
				UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Cannot add sort key that already exists in the array"));
			}
			return;
		}

		if (!ensureAlwaysMsgf(!SortedAndFilteredAddresses.Contains(Address), TEXT("Cannot add key that already exists. How did code get here?")))
		{
			return;
		}

		SortedAndFilteredAddresses.Insert(Address, Index);
	}

	NeedsReconstructEntries = true;
}

void UInventoryContentsBase::SetFilterByDelegate(const FBlueprintStorageFilter& Filter, const bool bResort)
{
	Query.Filter.Unbind();
	if (Filter.IsBound())
	{
		Query.Filter.BindLambda([Filter](const FFaerieItemSnapshot& Snapshot)
			{
				return Filter.Execute(Snapshot);
			});
	}

	if (bResort)
	{
		NeedsResort = true;
	}
}

void UInventoryContentsBase::ResetFilter(const bool bResort)
{
	UE_LOG(LogFaerieInventoryContent, Log, TEXT("Resetting to the default filter rule"));
	Query.Filter.BindUObject(this, &ThisClass::ExecFilter);
	ActiveFilterRule = DefaultFilterRule;
	if (bResort)
	{
		NeedsResort = true;
	}
}

void UInventoryContentsBase::SetSortRule(UFaerieItemDataComparator* Rule, const bool bResort)
{
	if (IsValid(Rule))
	{
		ActiveSortRule = Rule;
		if (bResort)
		{
			NeedsResort = true;
		}
	}
}

void UInventoryContentsBase::SetSortReverse(const bool Reverse, const bool bResort)
{
	if (Reverse != Query.InvertSort)
	{
		Query.InvertSort = Reverse;
		if (bResort)
		{
			NeedsResort = true;
		}
	}
}

void UInventoryContentsBase::ResetSort(const bool bResort)
{
	UE_LOG(LogFaerieInventoryContent, Log, TEXT("Resetting to the default sort rule"));
	Query.Sort.BindUObject(this, &ThisClass::ExecSort);
	ActiveSortRule = DefaultSortRule;
	if (bResort)
	{
		NeedsResort = true;
	}
}

#undef LOCTEXT_NAMESPACE