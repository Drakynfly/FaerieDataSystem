// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieStorageLibrary.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStorageLibrary)

FFaerieAddress UFaerieStorageLibrary::QueryFirst(UFaerieItemStorage* Storage, const FFaerieSnapshotPredicate& Filter)
{
	if (!IsValid(Storage) || !Filter.IsBound()) return FFaerieAddress();

	return Storage->QueryFirst(
		[Filter, Storage](const FFaerieAddress& Address)
		{
			FFaerieItemSnapshot Snapshot;
			const FFaerieItemStackView View = Storage->ViewStack(Address);
			Snapshot.Owner = Storage;
			Snapshot.ItemObject = View.Item.Get();
			Snapshot.Copies = View.Copies;
			return Filter.Execute(Snapshot);
		});
}
