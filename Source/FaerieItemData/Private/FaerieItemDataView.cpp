// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataView.h"
#include "FaerieItemOwnerInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataView)

void FFaerieItemDataView::SetItemObject(const Faerie::ItemData::FReference& Item)
{
	CachedInstance = Item;
}

void FFaerieItemDataView::SetCopies(const int32 Copies)
{
	CachedCopies = Copies;
}

void FFaerieItemDataView::SetOwner(const TNotNull<const IFaerieItemOwnerInterface*> Owner)
{
	CachedOwner = Owner;
}

FFaerieItemInstance FFaerieItemDataView::GetInstance() const
{
	if (CachedInstance.IsSet())
	{
		return CachedInstance.GetValue().GetInstance();
	}

	switch (Resolver.GetIndex())
	{
	case 1: // FValidatedProxy
		{
			const FValidatedProxy& Proxy = Resolver.Get<FValidatedProxy>();
			const FFaerieItemInstance Instance = Proxy->GetItemInstance().GetValue();
			CachedInstance = Instance;
			return Instance;
		}
	case 2: // FExternalResolver
		{
			const FExternalResolver& ExternalResolver = Resolver.Get<FExternalResolver>();
			const Faerie::ItemData::FReference ItemObject = ExternalResolver->ResolveItem();
			CachedInstance = ItemObject;
			return ItemObject.GetInstance();
		}
	default:
		return FFaerieItemInstance();
	}
}

int32 FFaerieItemDataView::GetCopies() const
{
	if (CachedCopies.IsSet())
	{
		return CachedCopies.GetValue();
	}

	switch (Resolver.GetIndex())
	{
	case 1: // FValidatedProxy
		{
			const FValidatedProxy& Proxy = Resolver.Get<FValidatedProxy>();
			const int32 Copies = Proxy->GetCopies();
			CachedCopies = Copies;
			return Copies;
		}
	case 2: // FExternalResolver
		{
			const FExternalResolver& ExternalResolver = Resolver.Get<FExternalResolver>();
			const int32 Copies = ExternalResolver->ResolveCopies();
			CachedCopies = Copies;
			return Copies;
		}
	default:
		return 0;
	}
}

const IFaerieItemOwnerInterface* FFaerieItemDataView::GetOwner() const
{
	if (CachedOwner.IsSet())
	{
		return CachedOwner.GetValue();
	}

	switch (Resolver.GetIndex())
	{
	case 1: // FValidatedProxy
		{
			const FValidatedProxy& Proxy = Resolver.Get<FValidatedProxy>();
			const IFaerieItemOwnerInterface* Owner = Proxy->GetItemOwner();
			CachedOwner = Owner;
			return Owner;
		}
	case 2: // FExternalResolver
		{
			const FExternalResolver& ExternalResolver = Resolver.Get<FExternalResolver>();
			const IFaerieItemOwnerInterface* Owner = ExternalResolver->ResolveOwner();
			CachedOwner = Owner;
			return Owner;
		}
	default:
		return nullptr;
	}
}