// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryHashStatics.h"
#include "FaerieContainerIterator.h"
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

		for (const UFaerieItem* Item : Container::ItemRange(Container))
		{
			Hashes.Add(Function(Item));
		}

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
			for (const UFaerieItem* Item : Container::ItemRange(Container))
			{
				Hashes.Add(Function(Item));
			}
		}

		return CombineHashes(Hashes);
	}
}