// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/FaerieStorageWidgetBase.h"
#include "UI/InventoryFillMeterBase.h"
#include "UI/InventoryUIAction.h"
#include "UI/InventoryUIActionContainer.h"

#include "FaerieInventoryContentLog.h"
#include "FaerieItemStorageQuery.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageWidgetBase)

#define LOCTEXT_NAMESPACE "FaerieStorageWidgetBase"

UFaerieStorageWidgetBase::UFaerieStorageWidgetBase(const FObjectInitializer& ObjectInitializer)
  : Super(ObjectInitializer)
{
	ActionContainer = CreateDefaultSubobject<UInventoryUIActionContainer>(TEXT("ActionContainer"));
	StorageQuery = CreateDefaultSubobject<UFaerieItemStorageQuery>(TEXT("StorageQuery"));
}

bool UFaerieStorageWidgetBase::Initialize()
{
	// Request Resort after any change to our Query Object.
	StorageQuery->GetQueryChangedEvent().AddWeakLambda(this, [this](auto){ RequestResort(); });

	return Super::Initialize();
}

void UFaerieStorageWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// Resort and display items whenever we are reconstructed with an existing inventory.
	if (ItemStorage.IsValid())
	{
		InitWithInventory(ItemStorage.Get());
	}
}

void UFaerieStorageWidgetBase::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (NeedsResort)
	{
		if (ItemStorage.IsValid())
		{
			StorageQuery->QueryAllAddresses(ItemStorage.Get(), SortedAndFilteredAddresses);
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

void UFaerieStorageWidgetBase::Reset()
{
	SortedAndFilteredAddresses.Empty();
	StorageQuery->SetInvertSort(false);
	StorageQuery->SetInvertFilter(false);

	if (ItemStorage.IsValid())
	{
		ItemStorage->GetOnAddressEvent().RemoveAll(this);
	}

	OnReset();

	ItemStorage = nullptr;
}

void UFaerieStorageWidgetBase::HandleAddressEvent(UFaerieItemStorage* Storage, const EFaerieAddressEventType Type, const FFaerieAddress Address)
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

void UFaerieStorageWidgetBase::SetLinkedStorage(UFaerieItemStorage* Storage)
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

void UFaerieStorageWidgetBase::InitWithInventory(UFaerieItemStorage* Storage)
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

void UFaerieStorageWidgetBase::AddToSortOrder(const FFaerieAddress Address, const bool WarnIfAlreadyExists)
{
	// If we are going to perform a full resort next frame anyway, then this is pointless.
	if (NeedsResort)
	{
		return;
	}

	// This address is filtered, skip adding to list.
	if (StorageQuery->IsAddressFiltered(ItemStorage.Get(), Address))
	{
		return;
	}

	if (SortedAndFilteredAddresses.IsEmpty())
	{
		SortedAndFilteredAddresses.Add(Address);
	}
	else
	{
		if (StorageQuery->IsSortBound())
		{
			auto SortPredicate = [this](const FFaerieAddress A, const FFaerieAddress B)
			{
				return StorageQuery->CompareAddresses(ItemStorage.Get(), A, B);
			};

			// Use binary search to find position to insert the new key.
			const int32 Index = Algo::LowerBound(SortedAndFilteredAddresses, Address, SortPredicate);

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
		else
		{
			UE_LOG(LogFaerieInventoryContent, Warning, TEXT("StorageQuery's Sort is invalid. Content will not be sorted!"));
			if (SortedAndFilteredAddresses.Find(Address) != INDEX_NONE)
			{
				if (WarnIfAlreadyExists)
				{
					UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Cannot add sort key that already exists in the array"));
				}
				return;
			}

			SortedAndFilteredAddresses.Add(Address);
		}
	}

	NeedsReconstructEntries = true;
}

void UFaerieStorageWidgetBase::RequestResort()
{
	NeedsResort = true;
}

#undef LOCTEXT_NAMESPACE
