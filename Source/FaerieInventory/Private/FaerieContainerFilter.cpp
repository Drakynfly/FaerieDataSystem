// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerFilter.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::Container
{
	// Declare the permutations of this template, so we can store the implementations in this file.
	template TMemoryFilter<false>;
	template TMemoryFilter<true>;

	namespace Private
	{
		UFaerieItem* FContainerReader::GetItem(const UFaerieItemContainerBase* Container, const FEntryKey Key)
		{
			return Container->View(Key).Item->MutateCast();
		}

		UFaerieItem* FContainerReader::GetItem(const UFaerieItemContainerBase* Container, const FFaerieAddress Address)
		{
			return Container->ViewItem(Address)->MutateCast();
		}

		const UFaerieItem* FContainerReader::ConstGetItem(const UFaerieItemContainerBase* Container, const FEntryKey Key)
		{
			return Container->View(Key).Item.Get();
		}

		const UFaerieItem* FContainerReader::ConstGetItem(const UFaerieItemContainerBase* Container, const FFaerieAddress Address)
		{
			return Container->ViewItem(Address);
		}

		FFaerieItemStackView FContainerReader::GetStackView(const UFaerieItemContainerBase* Container, const FEntryKey Key)
		{
			return Container->View(Key);
		}

		FFaerieItemStackView FContainerReader::GetStackView(const UFaerieItemContainerBase* Container, const FFaerieAddress Address)
		{
			return Container->ViewStack(Address);
		}

		FFaerieItemSnapshot FContainerReader::MakeSnapshot(
			const UFaerieItemContainerBase* Container, const FEntryKey Key)
		{
			FFaerieItemSnapshot Snapshot;
			Snapshot.Owner = Container;
			const FFaerieItemStackView View = Container->View(Key);
			Snapshot.ItemObject = View.Item.Get();
			Snapshot.Copies = View.Copies;
			return Snapshot;
		}

		FFaerieItemSnapshot FContainerReader::MakeSnapshot(
			const UFaerieItemContainerBase* Container, const FFaerieAddress Address)
		{
			FFaerieItemSnapshot Snapshot;
			Snapshot.Owner = Container;
			const FFaerieItemStackView View = Container->ViewStack(Address);
			Snapshot.ItemObject = View.Item.Get();
			Snapshot.Copies = View.Copies;
			return Snapshot;
		}

		FEntryKey FContainerReader::GetAddressEntry(const UFaerieItemContainerBase* Container, const FFaerieAddress Address)
		{
			return Container->FILTER_GetBaseKey(Address);
		}

		TArray<FFaerieAddress> FContainerReader::GetEntryAddresses(const UFaerieItemContainerBase* Container, const FEntryKey Key)
		{
			return Container->FILTER_GetKeyAddresses(Key);
		}
	}

	template <bool bAddress> void TMemoryFilter<bAddress>::Invert()
	{
		TSet<FFilterElement> NewSet;
		if constexpr (bAddress)
		{
			//FullSet.Reserve(Container->)
			for (auto Element : AddressRange(Container))
			{
				NewSet.Add(Element);
			}
		}
		else
		{
			//FullSet.Reserve(Container->)
			for (auto Element : KeyRange(Container))
			{
				NewSet.Add(Element);
			}
		}

		FilterMemory = NewSet.Difference(TSet<FFilterElement>(FilterMemory)).Array();
	}

	template <bool bAddress> void TMemoryFilter<bAddress>::Reset()
	{
		FilterMemory.Reset();
		if constexpr (bAddress)
		{
			//FullSet.Reserve(Container->)
			for (auto Element : AddressRange(Container))
			{
				FilterMemory.Add(Element);
			}
		}
		else
		{
			//FullSet.Reserve(Container->)
			for (auto Element : KeyRange(Container))
			{
				FilterMemory.Add(Element);
			}
		}
	}

	FKeyFilter KeyFilter(const UFaerieItemContainerBase* Container)
	{
		return FKeyFilter(Container->CreateFilter(false));
	}

	FAddressFilter AddressFilter(const UFaerieItemContainerBase* Container)
	{
		return FAddressFilter(Container->CreateFilter(true));
	}
}
