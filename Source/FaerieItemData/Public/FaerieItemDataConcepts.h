// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataFwd.h"
#include "Templates/RemoveReference.h"
#include "Templates/UnrealTypeTraits.h"

namespace Faerie::ItemData
{
	// Concept for any mass fragment type, either the abstract parent or any derived child.
	template <typename T>
	concept CFragmentBase = TIsDerivedFrom<typename TRemoveReference<T>::Type, FFaerieMassFragment>::Value;

	// Concept for an implementation of the mass fragment type.
	template <typename T>
	concept CFragmentImpl = TIsDerivedFrom<typename TRemoveReference<T>::Type, FFaerieMassFragment>::Value && !std::is_same_v<typename TRemoveReference<T>::Type, FFaerieMassFragment>;

	template <typename T>
	concept CItemDataProxy = TIsDerivedFrom<typename TRemoveReference<T>::Type, IFaerieItemDataProxy>::Value;
}