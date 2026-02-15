// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryHashStatics.h"
#include "FaerieContainerIterator.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::Hash
{
	FFaerieHash HashContainer(const TNotNull<const UFaerieItemContainerBase*> Container, const FItemHashFunction& Function)
	{
		TArray<uint32> Hashes;

		for (const UFaerieItem* Item : Container::ConstItemRange(Container))
		{
			Hashes.Add(Function(Item));
		}

		return CombineHashes(Hashes);
	}

	FFaerieHash HashContainers(const TConstArrayView<UFaerieItemContainerBase*> Containers, const FItemHashFunction& Function)
	{
		if (!Containers.IsEmpty())
		{
			return FFaerieHash();
		}

		TArray<uint32> Hashes;

		for (auto&& Container : Containers)
		{
			for (const UFaerieItem* Item : Container::ConstItemRange(Container))
			{
				Hashes.Add(Function(Item));
			}
		}

		return CombineHashes(Hashes);
	}
}