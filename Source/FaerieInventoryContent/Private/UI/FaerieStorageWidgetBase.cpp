// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/FaerieStorageWidgetBase.h"
#include "UI/InventoryUIAction.h"
#include "UI/InventoryUIActionContainer.h"

#include "FaerieInventoryContentLog.h"
#include "FaerieContainerQuery.h"

#include "Extensions/ItemContainerExtensionEvents.h"

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

void UFaerieStorageWidgetBase::Reset()
{
	SortedAndFilteredAddresses.Empty();
	if (IsValid(StorageQuery))
	{
		StorageQuery->SetInvertSort(false);
		StorageQuery->SetInvertFilter(false);
	}

	if (ItemStorage.IsValid())
	{
		if (auto EventsExtension = Extensions::Get<UItemContainerExtensionEvents>(ItemStorage->GetExtensions(), false))
		{
			EventsExtension->GetOnPostEventBatch().RemoveAll(this);
		}
	}

	OnReset();
}

void UFaerieStorageWidgetBase::OnPostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container,
	const Inventory::FEventLogBatch& Events)
{
	if (Container != ItemStorage) return;

	// If we are going to perform a full query next frame anyway, then this is pointless.
	if (NeedsNewQuery)
	{
		return;
	}

	// @todo do we need a way to customize this value
	if (Events.Data.Num() > 1)
	{
		RequestQuery();
		return;
	}

	if (Events.IsAdditionEvent())
	{
		for (auto&& Event : Events.Data)
        {
            for (auto&& Address : Event.AddressesTouched)
            {
            	const int32 Index = AddToSortOrder(Address, true);
            	if (Index != INDEX_NONE)
            	{
            		OnAddressAdded(Address, Index);
            	}
            }
        }
	}
	else if (Events.IsRemovalEvent())
	{
		for (auto&& Event : Events.Data)
		{
			for (auto&& Address : Event.AddressesTouched)
            {
				if (const int32 Index = SortedAndFilteredAddresses.Find(Address);
					Index != INDEX_NONE)
				{
					SortedAndFilteredAddresses.RemoveAt(Index);
					OnAddressRemoved(Address, Index);
				}
            }
		}
	}
	else
	{
		check(Events.IsEditEvent())

		for (auto&& Event : Events.Data)
		{
			for (auto&& Address : Event.AddressesTouched)
            {
            	const int32 Index = AddToSortOrder(Address, false);
            	if (Index != INDEX_NONE)
            	{
            		OnAddressAdded(Address, Index);
            	}
            	else
            	{
            		OnAddressUpdated(Address, INDEX_NONE);
            	}
            }
		}
	}
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

void UFaerieStorageWidgetBase::InitWithStorage()
{
	// Reset state fully.
    Reset();

	UFaerieItemStorage* Storage = ItemStorage.Get();

    if (ensure(IsValid(Storage)))
    {
        if (EnableUpdateEvents)
        {
        	auto EventsExtension = Extensions::Get<UItemContainerExtensionEvents>(Storage->GetExtensions(), false);
        	if (!IsValid(EventsExtension))
        	{
        		UE_LOG(LogFaerieInventoryContent, Error,
        			TEXT("Storage Widget failed to find Events Extension. Dynamic updates disabled! Please add a Extension Events object to '%s' or disable EnableUpdateEvents"),
        			*Storage->GetPathName())
        	}
        	else
        	{
        		EventsExtension->GetOnPostEventBatch().AddUObject(this, &ThisClass::OnPostEventBatch);
        	}
        }

        OnInitWithInventory();

        // Load in entries that should be initially displayed
        NeedsNewQuery = true;
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
				UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Cannot add address %lld that already exists in the array!"), Address.Address);
			}
			return INDEX_NONE;
		}

		if (int32 ExistingIndex = INDEX_NONE;
			SortedAndFilteredAddresses.Find(Address, ExistingIndex))
		{
			if (WarnIfAlreadyExists)
			{
				UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Cannot add address %lld at index '%i' that already exists at index '%i'. How did code get here?"), Address.Address, Index, ExistingIndex);
			}
			return INDEX_NONE;
		}

		return SortedAndFilteredAddresses.Insert(Address, Index);
	}

	UE_LOG(LogFaerieInventoryContent, Verbose, TEXT("StorageQuery's Sort is invalid. Content will not be sorted!"));
	if (SortedAndFilteredAddresses.Find(Address) != INDEX_NONE)
	{
		if (WarnIfAlreadyExists)
		{
			UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Cannot add address that already exists in the array"));
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
