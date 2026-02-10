// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerIterator.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::Container
{
	namespace Private
	{
		TUniquePtr<IIterator> FIteratorAccess::CreateEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container)
		{
			return Container->CreateEntryIterator();
		}

		TUniquePtr<IIterator> FIteratorAccess::CreateAddressIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container)
		{
			return Container->CreateAddressIterator();
		}

		TUniquePtr<IIterator> FIteratorAccess::CreateSingleEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container, const FEntryKey Key)
		{
			return Container->CreateSingleEntryIterator(Key);
		}
	}

	FKeyIterator KeyRange(const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FKeyIterator(Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}

	FAddressIterator AddressRange(const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FAddressIterator(Private::FIteratorAccess::CreateAddressIteratorImpl(Container));
	}

	FAddressIterator SingleKeyRange(const TNotNull<const UFaerieItemContainerBase*> Container, const FEntryKey Key)
	{
		return FAddressIterator(Private::FIteratorAccess::CreateSingleEntryIteratorImpl(Container, Key));
	}

	FConstItemIterator ConstItemRange(const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FConstItemIterator(Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}

	FItemIterator ItemRange(const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FItemIterator(Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}
}
