// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerExtensionInterface.h"
#include "ItemContainerExtensionBase.h"

#include "Templates/SubclassOf.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerExtensionInterface)

bool IFaerieContainerExtensionInterface::GetExtensionChecked(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass,
															 UItemContainerExtensionBase*& Extension, const bool RecursiveSearch) const
{
	const UItemContainerExtensionGroup* Group = VirtualGetExtensionGroup();
	Extension = Group->GetExtension(ExtensionClass, RecursiveSearch);
	return IsValid(Extension);
}

namespace Faerie::Extensions
{
	const UItemContainerExtensionBase* Get(const UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch)
	{
		return Group->GetExtension(Class, RecursiveSearch);
	}

	UItemContainerExtensionBase* Get(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch)
	{
		return Group->GetExtension(Class, RecursiveSearch);
	}

	UItemContainerExtensionBase* AddExtensionByClass(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> ExtensionClass)
	{
		if (!ensure(
			IsValid(ExtensionClass) &&
			ExtensionClass != UItemContainerExtensionBase::StaticClass()))
		{
			return nullptr;
		}

		UItemContainerExtensionBase* NewExtension = NewObject<UItemContainerExtensionBase>(Group, ExtensionClass);
		SET_NEW_IDENTIFIER(NewExtension, TEXTVIEW("NewExt:ContainerExtensionInterface"))
		Group->AddExtension(NewExtension);

		return NewExtension;
	}

	bool RemoveExtensionByClass(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> ExtensionClass, const bool RecursiveSearch)
	{
		if (!ensure(
				IsValid(ExtensionClass) &&
				ExtensionClass != UItemContainerExtensionBase::StaticClass()))
		{
			return false;
		}

		UItemContainerExtensionBase* Extension = Group->GetExtension(ExtensionClass, RecursiveSearch);
		if (!IsValid(Extension))
		{
			return false;
		}

		return Group->RemoveExtension(Extension);
	}
}
