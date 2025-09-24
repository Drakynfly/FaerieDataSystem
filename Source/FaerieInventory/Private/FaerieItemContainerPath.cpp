// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerPath.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieItemContainerBase.h"
#include "Tokens/FaerieItemStorageToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerPath)

void BuildPath_Recurse(UFaerieItemContainerBase* Container, const FFaerieItemContainerPath& BasePath, TArray<FFaerieItemContainerPath>& OutPaths)
{
	FFaerieItemContainerPath& NewPath = OutPaths.Emplace_GetRef(BasePath);
	NewPath.Containers.Add(Container);

	for (UFaerieItem* Item : Faerie::KeyFilter(Container).Run<Faerie::FMutableFilter>().Items())
	{
		UFaerieItemContainerToken::ForEachContainer(Item,
			[&](UFaerieItemContainerBase* SubContainer)
			{
				BuildPath_Recurse(SubContainer, NewPath, OutPaths);
				return Faerie::Continue;
			}, false);
	}
}

void FFaerieItemContainerPath::BuildChildrenPaths(UFaerieItemContainerBase* Head, TArray<FFaerieItemContainerPath>& OutPaths)
{
	BuildPath_Recurse(Head, FFaerieItemContainerPath(), OutPaths);
}

FFaerieItemContainerPath FFaerieItemContainerPath::BuildParentPath(UFaerieItemContainerBase* Tail)
{
	FFaerieItemContainerPath Path;

	UFaerieItemContainerBase* Container = Tail;

	while (IsValid(Container))
	{
		Path.Containers.Add(Container);
		Container = Container->GetTypedOuter<UFaerieItemContainerBase>();
	}

	return Path;
}
