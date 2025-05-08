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

		Container->ForEachItem(
			[&Hashes, &Function](const UFaerieItem* Item)
			{
				Hashes.Add(Function(Item));
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
			Container->ForEachItem(
				[&Hashes, &Function](const UFaerieItem* Item)
				{
					Hashes.Add(Function(Item));
				});
		}

		return CombineHashes(Hashes);
	}
}