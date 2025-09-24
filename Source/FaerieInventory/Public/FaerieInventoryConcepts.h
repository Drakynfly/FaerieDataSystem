// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Templates/RemoveReference.h"
#include "Templates/UnrealTypeTraits.h"

class UItemContainerExtensionBase;

namespace Faerie
{
	template <typename T>
	concept CItemContainerExtension = TIsDerivedFrom<typename TRemoveReference<T>::Type, UItemContainerExtensionBase>::Value;
}