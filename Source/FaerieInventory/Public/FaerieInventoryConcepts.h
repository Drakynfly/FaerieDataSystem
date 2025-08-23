// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

class UItemContainerExtensionBase;

namespace Faerie
{
	template<typename T>
	concept CItemContainerExtension = TIsDerivedFrom<typename TRemoveReference<T>::Type, UItemContainerExtensionBase>::Value;
}