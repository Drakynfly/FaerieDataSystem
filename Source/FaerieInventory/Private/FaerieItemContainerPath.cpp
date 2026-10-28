// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerPath.h"
#include "FaerieItemContainerBase.h"
#include "FaerieContainerIterator.h"
#include "FaerieSubObjectFilter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerPath)

using namespace Faerie;

namespace Faerie::Container
{
	static void BuildPath_Recurse(const FMassEntityManager* EntityManager, const FFaerieItemProxy& Owner,
	   const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemContainerPath& BasePath, TArray<FFaerieItemContainerPath>& OutPaths)
	{
		FFaerieItemContainerPath& NewPath = OutPaths.Emplace_GetRef(BasePath);
		NewPath.Containers.Add({
			.OwningProxy = Owner,
			.Container = Container
		});

		// For each item in this container...
		for (auto It = MutableItemRange(EntityManager, Container); It; ++It)
		{
			// Since we emit this proxy in the container path, we need a persistent one.
			const FFaerieItemProxy Proxy = It.GetPersistentProxy();

			// Iterate sub-containers in that item, and build another path from it.
			for (UFaerieItemContainerBase* SubContainer : SubObject::Iterate(Proxy))
			{
				BuildPath_Recurse(EntityManager, Proxy, SubContainer, NewPath, OutPaths);
			}
		}
	}
}

void FFaerieItemContainerPath::BuildChildrenPaths(const FMassEntityManager* EntityManager,
	const TNotNull<UFaerieItemContainerBase*> Head, TArray<FFaerieItemContainerPath>& OutPaths)
{
	Container::BuildPath_Recurse(EntityManager, FFaerieItemProxy(), Head, FFaerieItemContainerPath(), OutPaths);
}