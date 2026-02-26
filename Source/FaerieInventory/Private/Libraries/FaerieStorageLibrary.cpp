// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieStorageLibrary.h"
#include "DelegateCommon.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieItemDataViewWrapper.h"
#include "FaerieItemStorage.h"
#include "FaerieItemStorageIterators.h"
#include "ItemStackProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageLibrary)

const UFaerieItem* UFaerieStorageLibrary::GetViewItem(const FFaerieItemDataViewWrapper& View)
{
	return View.ViewPointer->ResolveItem();
}

int32 UFaerieStorageLibrary::GetViewCopies(const FFaerieItemDataViewWrapper& View)
{
	return View.ViewPointer->ResolveView().Copies;
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieStorageLibrary::GetViewOwner(const FFaerieItemDataViewWrapper& View)
{
	// Not const safe, but only so much we can do with BP the way it is...
	return const_cast<UObject*>(Cast<UObject>(View.ViewPointer->ResolveOwner()));
}

TArray<UFaerieItemStackProxy*> UFaerieStorageLibrary::GetAllStackProxies(UFaerieItemStorage* Storage)
{
	if (!IsValid(Storage)) return {};

	TArray<UFaerieItemStackProxy*> Proxies;
	Proxies.Reserve(Storage->GetStackCount());
	for (auto It = Faerie::Storage::FIterator_AllAddresses(Storage); It; ++It)
	{
		Proxies.Add(const_cast<UFaerieItemStackProxy*>(Cast<UFaerieItemStackProxy>(Storage->Proxy(*It).GetObject())));
	}
	return Proxies;
}

FFaerieAddress UFaerieStorageLibrary::QueryFirst(UFaerieItemStorage* Storage, const FFaerieViewPredicate& Filter)
{
	using namespace Faerie::Container;

	if (!IsValid(Storage) || !Filter.IsBound()) return FFaerieAddress();

	return FAddressFilter()
		.By(FCallbackFilter{DYNAMIC_TO_NATIVE(FIteratorPredicate, Filter)})
		.First(Storage);
}
