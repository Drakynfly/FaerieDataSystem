// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerFilterTypes.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemStackView.h"

namespace Faerie::Container
{
	bool FSingleKey::Passes(const FEntryKey Key)
	{
		return Key == TestKey;
	}

	bool FKeySet::Passes(const FEntryKey Key)
	{
		return TestKeys.Contains(Key);
	}

	bool FMutableFilter::StaticPasses(const UFaerieItem* Item)
	{
		return Item->CanMutate();
	}

	bool FImmutableFilter::StaticPasses(const UFaerieItem* Item)
	{
		return !Item->CanMutate();
	}

	bool FSnapshotFilterObj::Passes(const FFaerieItemSnapshot& Snapshot)
	{
		if (FilterObj)
		{
			FFaerieItemStackView View;
			View.Item = Snapshot.ItemObject;
			View.Copies = Snapshot.Copies;
			return FilterObj->Exec(View);
		}
		return false;
	}

	bool FSnapshotFilterCallback::Passes(const FFaerieItemSnapshot& Snapshot)
	{
		if (Callback.IsBound())
		{
			return Callback.Execute(Snapshot);
		}
		return false;
	}
}
