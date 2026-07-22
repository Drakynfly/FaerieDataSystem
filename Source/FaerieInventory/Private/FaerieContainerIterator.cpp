// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerIterator.h"

namespace Faerie::Container
{
	namespace Private
	{
		TUniquePtr<IEntryIterator> FIteratorAccess::CreateEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container)
		{
			return Container->CreateEntryIterator();
		}

		TUniquePtr<IAddressIterator> FIteratorAccess::CreateAddressIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container)
		{
			return Container->CreateAddressIterator();
		}

		TUniquePtr<IAddressIterator> FIteratorAccess::CreateSingleEntryIteratorImpl(const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key)
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

	FAddressIterator SingleKeyRange(const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key)
	{
		return FAddressIterator(Private::FIteratorAccess::CreateSingleEntryIteratorImpl(Container, Key));
	}

	FItemIterator ItemRange(const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FItemIterator(Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}

	FMutableItemIterator MutableItemRange(const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FMutableItemIterator(Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}
}
