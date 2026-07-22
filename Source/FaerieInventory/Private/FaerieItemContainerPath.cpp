// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerPath.h"
#include "FaerieItemContainerBase.h"
#include "FaerieContainerIterator.h"
#include "FaerieSubObjectFilter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerPath)

using namespace Faerie;

namespace Faerie::Container
{
	void BuildPath_Recurse(ItemData::FRequireEntityManager& EntityManager, const ItemData::FMutableReference& Owner,
	   const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemContainerPath& BasePath, TArray<FFaerieItemContainerPath>& OutPaths)
	{
		FFaerieItemContainerPath& NewPath = OutPaths.Emplace_GetRef(BasePath);
		NewPath.Containers.Add({Owner.GetInstance(), Container});

		for (auto It = MutableItemRange(Container); It; ++It)
		{
			ItemData::FMutableReference Instance = *It;
			for (UFaerieItemContainerBase* SubContainer : SubObject::Iterate(EntityManager, *It))
			{
				BuildPath_Recurse(EntityManager, Instance, SubContainer, NewPath, OutPaths);
			}
		}
	}
}

void FFaerieItemContainerPath::BuildChildrenPaths(ItemData::FRequireEntityManager& EntityManager,
	const ItemData::FMutableReference& Owner, const TNotNull<UFaerieItemContainerBase*> Head, TArray<FFaerieItemContainerPath>& OutPaths)
{
	Container::BuildPath_Recurse(EntityManager, Owner, Head, FFaerieItemContainerPath(), OutPaths);
}