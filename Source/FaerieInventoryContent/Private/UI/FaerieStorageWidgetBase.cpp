// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/FaerieStorageWidgetBase.h"
#include "UI/InventoryUIActionContainer.h"

#include "FaerieContainerEvent.h"
#include "FaerieInventoryContentLog.h"
#include "FaerieContainerQuery.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageWidgetBase)

#define LOCTEXT_NAMESPACE "FaerieStorageWidgetBase"

using namespace Faerie;

UFaerieStorageWidgetBase::UFaerieStorageWidgetBase(const FObjectInitializer& ObjectInitializer)
  : Super(ObjectInitializer)
{
	ActionContainer = CreateDefaultSubobject<UInventoryUIActionContainer>(TEXT("ActionContainer"));
	ActionContainer->SetFlags(RF_Transactional | RF_ArchetypeObject);

	StorageQuery = CreateDefaultSubobject<UFaerieContainerQuery>(TEXT("StorageQuery"));
	StorageQuery->SetFlags(RF_Transactional | RF_ArchetypeObject);
}

bool UFaerieStorageWidgetBase::Initialize()
{
	// Request Resort after any change to our Query Object.
	StorageQuery->GetQueryChangedEvent().AddWeakLambda(this, [this](auto){ RequestQuery(); });

	return Super::Initialize();
}

void UFaerieStorageWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// Resort and display items whenever we are reconstructed with an existing inventory.
	if (ItemStorage.IsValid())
	{
		InitWithStorage();
	}
}

void UFaerieStorageWidgetBase::NativeDestruct()
{
	// ItemStorage is not cleared on Destruct so we still have it for rebuilding in NativeConstruct
	Reset();
	Super::NativeDestruct();
}

void UFaerieStorageWidgetBase::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (NeedsNewQuery)
	{
		StorageQuery->QueryAllAddresses(ItemStorage.Get(), SortedAndFilteredAddresses);
		NeedsReDisplay = true;
		NeedsNewQuery = false;
	}

	if (NeedsReDisplay)
	{
		DisplayAddresses();
		NeedsReDisplay = false;
	}
}

void UFaerieStorageWidgetBase::OnContainerEventBatch(TNotNull<UFaerieItemContainerBase*> Container, const TConstArrayView<const Container::FEvent*> Events)
{
	// If we are going to perform a full query next frame anyway, then this is pointless.
	if (NeedsNewQuery)
	{
		return;
	}

	// @todo do we need a way to customize this value
	if (Events.Num() > 1)
	{
		RequestQuery();
		return;
	}

	for (auto&& Event : Events)
	{
		if (Event->IsAdditionEvent())
		{
			for (auto&& Address : Event->AddressesTouched)
			{
				// @Todo we don't know if the address was added "to" or added for the first time. separate "Addition" tag
				// into "NewEntry" versus "CountIncrement".
				int32 Index = SortedAndFilteredAddresses.Find(Address);
				if (Index != INDEX_NONE)
				{
					OnAddressUpdated(Address, Index);
				}
				else
				{
					Index = AddToSortOrder(Address, true);
					if (Index != INDEX_NONE)
					{
						OnAddressAdded(Address, Index);
					}
				}
			}
		}
		else if (Event->IsRemovalEvent())
		{
			for (auto&& Address : Event->AddressesTouched)
            {
                if (const int32 Index = SortedAndFilteredAddresses.Find(Address);
                	Index != INDEX_NONE)
                {
                	SortedAndFilteredAddresses.RemoveAt(Index);
                	OnAddressRemoved(Address, Index);
                }
            }
		}
		else
		{
			check(Event->IsEditEvent())

			for (auto&& Address : Event->AddressesTouched)
			{
				int32 Index = AddToSortOrder(Address, false);
				if (Index != INDEX_NONE)
				{
					OnAddressAdded(Address, Index);
				}
				else
				{
					Index = SortedAndFilteredAddresses.Find(Address);
					OnAddressUpdated(Address, Index);
				}
			}
		}
	}
}

void UFaerieStorageWidgetBase::InitWithStorage()
{
	// Reset state fully.
	Reset();

	UFaerieItemStorage* Storage = ItemStorage.Get();

	if (ensure(IsValid(Storage)))
	{
		if (EnableUpdateEvents)
		{
			Content::SubscribeToContainerEvents(Storage, this);
		}

		OnInitWithInventory();

		// Load in entries that should be initially displayed
		NeedsNewQuery = true;
	}
}

void UFaerieStorageWidgetBase::Reset()
{
	SortedAndFilteredAddresses.Empty();
	if (IsValid(StorageQuery))
	{
		StorageQuery->SetInvertSort(false);
		StorageQuery->SetInvertFilter(false);
	}

	if (UFaerieItemStorage* Storage = ItemStorage.Get())
	{
		Content::UnsubscribeFromContainerEvents(Storage, this);
	}

	OnReset();
}

void UFaerieStorageWidgetBase::SetLinkedStorage(UFaerieItemStorage* Storage)
{
	if (ItemStorage != Storage)
	{
		ItemStorage = Storage;

		if (IsConstructed())
        {
			if (Storage)
			{
				InitWithStorage();
			}
			else
			{
				Reset();
			}
        }
	}
}

int32 UFaerieStorageWidgetBase::AddToSortOrder(const FFaerieAddress Address, const bool WarnIfAlreadyExists)
{
	// This address is filtered, skip adding to list.
	if (StorageQuery->IsAddressFiltered(ItemStorage.Get(), Address))
	{
		return INDEX_NONE;
	}

	if (SortedAndFilteredAddresses.IsEmpty())
	{
		return SortedAndFilteredAddresses.Add(Address);
	}

	if (StorageQuery->IsSortBound())
	{
		auto SortPredicate = [this](const FFaerieAddress A, const FFaerieAddress B)
		{
			return StorageQuery->CompareAddresses(ItemStorage.Get(), A, B);
		};

		// Use binary search to find position to insert the new address.
		const int32 Index = Algo::LowerBound(SortedAndFilteredAddresses, Address, SortPredicate);

		// Return if the address we were sorted to or above is ourself.
		if ((SortedAndFilteredAddresses.IsValidIndex(Index) && (SortedAndFilteredAddresses[Index] == Address)) ||
			((SortedAndFilteredAddresses.IsValidIndex(Index+1) && (SortedAndFilteredAddresses[Index+1] == Address))))
		{
			if (WarnIfAlreadyExists)
			{
				UE_LOGF(LogFaerieInventoryContent, Warning, "Cannot add address %lld that already exists in the array!", Address.Address);
			}
			return INDEX_NONE;
		}

		if (int32 ExistingIndex = INDEX_NONE;
			SortedAndFilteredAddresses.Find(Address, ExistingIndex))
		{
			if (WarnIfAlreadyExists)
			{
				UE_LOGF(LogFaerieInventoryContent, Warning, "Cannot add address %lld at index '%i' that already exists at index '%i'. How did code get here?", Address.Address, Index, ExistingIndex);
			}
			return INDEX_NONE;
		}

		return SortedAndFilteredAddresses.Insert(Address, Index);
	}

	UE_LOGF(LogFaerieInventoryContent, Verbose, "StorageQuery's Sort is invalid. Content will not be sorted!");
	if (SortedAndFilteredAddresses.Find(Address) != INDEX_NONE)
	{
		if (WarnIfAlreadyExists)
		{
			UE_LOGF(LogFaerieInventoryContent, Warning, "Cannot add address that already exists in the array");
		}
		return INDEX_NONE;
	}

	return SortedAndFilteredAddresses.Add(Address);
}

void UFaerieStorageWidgetBase::RequestQuery()
{
	NeedsNewQuery = true;
}

#undef LOCTEXT_NAMESPACE
