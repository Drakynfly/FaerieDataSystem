// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieHashStatics.h"

class UFaerieItemContainerBase;

namespace Faerie::Hash
{
	FAERIEINVENTORY_API FFaerieHash HashContainer(const UFaerieItemContainerBase* Container, const FItemHashFunction& Function);
	FAERIEINVENTORY_API FFaerieHash HashContainers(const TConstArrayView<const UFaerieItemContainerBase*> Containers, const FItemHashFunction& Function);
}
