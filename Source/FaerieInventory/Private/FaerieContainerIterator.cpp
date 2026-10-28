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

	FKeyIterator KeyRange(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FKeyIterator(EntityManager, Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}

	FAddressIterator AddressRange(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FAddressIterator(EntityManager, Private::FIteratorAccess::CreateAddressIteratorImpl(Container));
	}

	FAddressIterator SingleKeyRange(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key)
	{
		return FAddressIterator(EntityManager, Private::FIteratorAccess::CreateSingleEntryIteratorImpl(Container, Key));
	}

	FItemIterator ItemRange(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FItemIterator(EntityManager, Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}

	FMutableItemIterator MutableItemRange(const FMassEntityManager* EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		return FMutableItemIterator(EntityManager, Private::FIteratorAccess::CreateEntryIteratorImpl(Container));
	}
}
