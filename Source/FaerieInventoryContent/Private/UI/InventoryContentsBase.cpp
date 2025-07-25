// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryContentsBase.h"
#include "UI/InventoryFillMeterBase.h"
#include "UI/InventoryUIAction.h"

#include "FaerieItemDataComparator.h"
#include "FaerieItemDataFilter.h"
#include "Actions/FaerieInventoryClient.h"

#include "Components/PanelWidget.h"
#include "UI/InventoryUIActionContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryContentsBase)

#define LOCTEXT_NAMESPACE "InventoryContentsBase"

DEFINE_LOG_CATEGORY(LogInventoryContents)

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
		ItemStorage->GetOnAddressAdded().RemoveAll(this);
		ItemStorage->GetOnAddressUpdated().RemoveAll(this);
		ItemStorage->GetOnAddressRemoved().RemoveAll(this);
	}

	OnReset();

	ItemStorage = nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeConst
// Cannot be const to bind
bool UInventoryContentsBase::ExecFilter(const FFaerieItemProxy& Entry)
{
	if (IsValid(ActiveFilterRule))
	{
		return ActiveFilterRule->Exec(Entry);
	}
	return Entry.IsValid();
}

// ReSharper disable once CppMemberFunctionMayBeConst
// Cannot be const to bind
bool UInventoryContentsBase::ExecSort(const FFaerieItemProxy& A, const FFaerieItemProxy& B)
{
	if (!IsValid(ActiveSortRule)) return false;
	return ActiveSortRule->Exec(A, B);
}

void UInventoryContentsBase::NativeAddressAdded(UFaerieItemStorage* Storage, const FFaerieAddress Address)
{
	if (bAlwaysAddNewToSortOrder)
	{
		AddToSortOrder(Address, true);
		OnKeyAdded(Address);
	}
}

void UInventoryContentsBase::NativeAddressUpdated(UFaerieItemStorage* Storage, const FFaerieAddress Address)
{
	if (bAlwaysAddNewToSortOrder)
	{
		AddToSortOrder(Address, false);
		OnKeyUpdated(Address);
	}
}

void UInventoryContentsBase::NativeAddressRemoved(UFaerieItemStorage* Storage, const FFaerieAddress Address)
{
	if (!!SortedAndFilteredAddresses.Remove(Address))
	{
		OnKeyRemoved(Address);
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

		ItemStorage->GetOnAddressAdded().AddUObject(this, &ThisClass::NativeAddressAdded);
		ItemStorage->GetOnAddressUpdated().AddUObject(this, &ThisClass::NativeAddressUpdated);
		ItemStorage->GetOnAddressRemoved().AddUObject(this, &ThisClass::NativeAddressRemoved);

		OnInitWithInventory();

		// Load in entries that should be initially displayed
		NeedsResort = true;
	}
}

void UInventoryContentsBase::SetInventoryClient(UFaerieInventoryClient* Client)
{
	InventoryClient = Client;
}

void UInventoryContentsBase::AddToSortOrder(const FFaerieAddress Address, const bool WarnIfAlreadyExists)
{
	struct FInsertKeyPredicate
	{
		FInsertKeyPredicate(const Faerie::FStorageQuery& Query, const UFaerieItemStorage* Storage)
		  : Query(Query),
			Storage(Storage) {}

		const Faerie::FStorageQuery& Query;
		const UFaerieItemStorage* Storage;

		bool operator()(const FFaerieAddress A, const FFaerieAddress B) const
		{
			const bool Result = Query.Sort.Execute(Storage->Proxy(A), Storage->Proxy(B));
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
			UE_LOG(LogInventoryContents, Verbose, TEXT("ActiveSortRule is invalid. Content will not be sorted!"));
			if (SortedAndFilteredAddresses.Find(Address) != INDEX_NONE)
			{
				if (WarnIfAlreadyExists)
				{
					UE_LOG(LogInventoryContents, Warning, TEXT("Cannot add sort key that already exists in the array"));
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
				UE_LOG(LogInventoryContents, Warning, TEXT("Cannot add sort key that already exists in the array"));
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
		Query.Filter.BindLambda([Filter](const FFaerieItemProxy& Proxy)
			{
				return Filter.Execute(Proxy);
			});
	}

	if (bResort)
	{
		NeedsResort = true;
	}
}

void UInventoryContentsBase::ResetFilter(const bool bResort)
{
	UE_LOG(LogInventoryContents, Log, TEXT("Resetting to the default filter rule"));
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
	UE_LOG(LogInventoryContents, Log, TEXT("Resetting to the default sort rule"));
	Query.Sort.BindUObject(this, &ThisClass::ExecSort);
	ActiveSortRule = DefaultSortRule;
	if (bResort)
	{
		NeedsResort = true;
	}
}

#undef LOCTEXT_NAMESPACE