// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerPath.h"
#include "FaerieItemContainerBase.h"
#include "FaerieContainerIterator.h"
#include "FaerieSubObjectFilter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerPath)

using namespace Faerie;

namespace Faerie::Container
{
	static void BuildPath_Recurse(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Owner,
	   const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemContainerPath& BasePath, TArray<FFaerieItemContainerPath>& OutPaths)
	{
		FFaerieItemContainerPath& NewPath = OutPaths.Emplace_GetRef(BasePath);
		NewPath.Containers.Add({
			.OwningInstance = Owner,
			.Container = Container
		});

		for (auto It = MutableItemRange(Container); It; ++It)
		{
			const FFaerieItemInstance Instance = *It;
			for (UFaerieItemContainerBase* SubContainer : SubObject::Iterate(EntityManager, Instance))
			{
				BuildPath_Recurse(EntityManager, Instance, SubContainer, NewPath, OutPaths);
			}
		}
	}
}

void FFaerieItemContainerPath::BuildChildrenPaths(const FMassEntityManager& EntityManager,
	const TValid<const FFaerieItemInstance&> Owner, const TNotNull<UFaerieItemContainerBase*> Head, TArray<FFaerieItemContainerPath>& OutPaths)
{
	Container::BuildPath_Recurse(EntityManager, Owner, Head, FFaerieItemContainerPath(), OutPaths);
}