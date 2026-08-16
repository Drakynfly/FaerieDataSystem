// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryHashStatics.h"
#include "FaerieContainerIterator.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::Hash
{
	FFaerieHash HashContainer(const TNotNull<const UFaerieItemContainerBase*> Container, const FMassEntityManager* EntityManager, const FItemHashFunction& Function)
	{
		TArray<uint32> Hashes;

		for (auto It = Container::ItemRange(Container); It; ++It)
		{
			Hashes.Add(Function(EntityManager, *It));
		}

		return CombineHashes(Hashes);
	}

	FFaerieHash HashContainers(const TConstArrayView<UFaerieItemContainerBase*> Containers, const FMassEntityManager* EntityManager, const FItemHashFunction& Function)
	{
		if (!Containers.IsEmpty())
		{
			return FFaerieHash();
		}

		TArray<uint32> Hashes;

		for (auto&& Container : Containers)
		{
			for (auto It = Container::ItemRange(Container); It; ++It)
			{
				Hashes.Add(Function(EntityManager, *It));
			}
		}

		return CombineHashes(Hashes);
	}
}