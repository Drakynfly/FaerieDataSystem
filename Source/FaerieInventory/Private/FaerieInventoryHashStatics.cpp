// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryHashStatics.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::Hash
{
	FFaerieHash HashContainer(const UFaerieItemContainerBase* Container, const FItemHashFunction& Function)
	{
		if (!IsValid(Container))
		{
			return FFaerieHash();
		}

		TArray<uint32> Hashes;

		Container->ForEachKey(
			[Container, &Hashes, &Function](const FEntryKey Key)
			{
				Hashes.Add(Function(Container->View(Key).Item.Get()));
			});

		return CombineHashes(Hashes);
	}

	FFaerieHash HashContainers(const TConstArrayView<const UFaerieItemContainerBase*> Containers, const FItemHashFunction& Function)
	{
		if (!Containers.IsEmpty())
		{
			return FFaerieHash();
		}

		TArray<uint32> Hashes;

		for (auto&& Container : Containers)
		{
			Container->ForEachKey(
				[Container, &Hashes, &Function](const FEntryKey Key)
				{
					Hashes.Add(Function(Container->View(Key).Item.Get()));
				});
		}

		return CombineHashes(Hashes);
	}
}