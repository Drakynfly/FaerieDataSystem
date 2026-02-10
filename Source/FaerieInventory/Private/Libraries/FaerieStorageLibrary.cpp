// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieStorageLibrary.h"
#include "DelegateCommon.h"
#include "FaerieContainerFilter.h"
#include "FaerieItemDataViewWrapper.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageLibrary)

const UFaerieItem* UFaerieStorageLibrary::GetViewItem(const FFaerieItemDataViewWrapper& View)
{
	return View.ViewPointer->ResolveItem();
}

int32 UFaerieStorageLibrary::GetViewCopies(const FFaerieItemDataViewWrapper& View)
{
	return View.ViewPointer->ResolveView().Copies;
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieStorageLibrary::GetViewOwner(
	const FFaerieItemDataViewWrapper& View)
{
	// Not const safe, but only so much we can do with BP the way it is...
	return const_cast<UObject*>(Cast<UObject>(View.ViewPointer->ResolveOwner()));
}

FFaerieAddress UFaerieStorageLibrary::QueryFirst(UFaerieItemStorage* Storage, const FFaerieViewPredicate& Filter)
{
	using namespace Faerie::Container;

	if (!IsValid(Storage) || !Filter.IsBound()) return FFaerieAddress();

	return FAddressFilter()
		.By(FCallbackFilter{DYNAMIC_TO_NATIVE(FIteratorPredicate, Filter)})
		.First(Storage);
}
