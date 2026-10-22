// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerPath.h"
#include "FaerieItemContainerBase.h"
#include "FaerieContainerIterator.h"
#include "FaerieSubObjectFilter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerPath)

using namespace Faerie;

namespace Faerie::Container
{
	static void BuildPath_Recurse(FMassEntityManager& EntityManager, const FFaerieItemProxy& Owner,
	   const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemContainerPath& BasePath, TArray<FFaerieItemContainerPath>& OutPaths)
	{
		FFaerieItemContainerPath& NewPath = OutPaths.Emplace_GetRef(BasePath);
		NewPath.Containers.Add({
			.OwningProxy = Owner,
			.Container = Container
		});

		// For each item in this container...
		for (auto It = MutableItemRange(Container); It; ++It)
		{
			const FFaerieItemProxy Proxy = It.GetPersistentProxy(EntityManager);

			// Iterate sub-containers in that item, and build another path from it.
			for (UFaerieItemContainerBase* SubContainer : SubObject::Iterate(EntityManager, Proxy.GetEntityHandle()))
			{
				BuildPath_Recurse(EntityManager, Proxy, SubContainer, NewPath, OutPaths);
			}
		}
	}
}

void FFaerieItemContainerPath::BuildChildrenPaths(FMassEntityManager& EntityManager,
	const FFaerieItemProxy& Owner, const TNotNull<UFaerieItemContainerBase*> Head, TArray<FFaerieItemContainerPath>& OutPaths)
{
	Container::BuildPath_Recurse(EntityManager, Owner, Head, FFaerieItemContainerPath(), OutPaths);
}