// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Templates/RemoveReference.h"
#include "Templates/UnrealTypeTraits.h"

class IFaerieItemDataProxy;
class UFaerieItem;
class UFaerieItemToken;

namespace Faerie
{
	template <typename T>
	concept CItem = TIsDerivedFrom<typename TRemoveReference<T>::Type, UFaerieItem>::Value;

	// Concept for any token class, either the abstract parent or any derived class.
	template <typename T>
	concept CItemTokenBase = TIsDerivedFrom<typename TRemoveReference<T>::Type, UFaerieItemToken>::Value;

	// Concept for an implementation of the token class.
	template <typename T>
	concept CItemTokenImpl = TIsDerivedFrom<typename TRemoveReference<T>::Type, UFaerieItemToken>::Value && !std::is_same_v<typename TRemoveReference<T>::Type, UFaerieItemToken>;

	template <typename T>
	concept CItemDataProxy = TIsDerivedFrom<typename TRemoveReference<T>::Type, IFaerieItemDataProxy>::Value;
}