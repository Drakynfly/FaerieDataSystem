// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Templates/RemoveReference.h"
#include "Templates/UnrealTypeTraits.h"

class UFaerieItemContainerBase;

namespace Faerie::Container
{
	template <typename T>
	concept CItemContainerBase = TIsDerivedFrom<typename TRemoveReference<T>::Type, UFaerieItemContainerBase>::Value;
}