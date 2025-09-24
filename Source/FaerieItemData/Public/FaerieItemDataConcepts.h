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

	template <typename T>
	concept CItemToken = TIsDerivedFrom<typename TRemoveReference<T>::Type, UFaerieItemToken>::Value;

	template <typename T>
	concept CItemDataProxy = TIsDerivedFrom<typename TRemoveReference<T>::Type, IFaerieItemDataProxy>::Value;
}