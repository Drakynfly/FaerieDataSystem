// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerExtensionInterface.h"
#include "ItemContainerExtensionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerExtensionInterface)

bool IFaerieContainerExtensionInterface::RemoveExtensionByClass(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass, const bool RecursiveSearch)
{
	if (!ensure(
			IsValid(ExtensionClass) &&
			ExtensionClass != UItemContainerExtensionBase::StaticClass()))
	{
		return false;
	}

	UItemContainerExtensionBase* Extension = GetExtension(ExtensionClass, RecursiveSearch);
	if (!IsValid(Extension))
	{
		return false;
	}

	return RemoveExtension(Extension);
}

bool IFaerieContainerExtensionInterface::HasExtension(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass, const bool RecursiveSearch) const
{
	return GetExtensionGroup()->HasExtension(ExtensionClass, RecursiveSearch);
}

UItemContainerExtensionBase* IFaerieContainerExtensionInterface::GetExtension(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass, const bool RecursiveSearch) const
{
	return GetExtensionGroup()->GetExtension(ExtensionClass, RecursiveSearch);
}

bool IFaerieContainerExtensionInterface::AddExtension(UItemContainerExtensionBase* Extension)
{
	return GetExtensionGroup()->AddExtension(Extension);
}

UItemContainerExtensionBase* IFaerieContainerExtensionInterface::AddExtensionByClass(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass)
{
	if (!ensure(
		IsValid(ExtensionClass) &&
		ExtensionClass != UItemContainerExtensionBase::StaticClass()))
	{
		return nullptr;
	}

	UItemContainerExtensionBase* NewExtension = NewObject<UItemContainerExtensionBase>(GetExtensionGroup(), ExtensionClass);
	SET_NEW_IDENTIFIER(NewExtension, TEXTVIEW("NewExt:ContainerExtensionInterface"))
	AddExtension(NewExtension);

	return NewExtension;
}

bool IFaerieContainerExtensionInterface::RemoveExtension(UItemContainerExtensionBase* Extension)
{
	return GetExtensionGroup()->RemoveExtension(Extension);
}

bool IFaerieContainerExtensionInterface::GetExtensionChecked(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass,
															 UItemContainerExtensionBase*& Extension, const bool RecursiveSearch) const
{
	Extension = GetExtension(ExtensionClass, RecursiveSearch);
	return IsValid(Extension);
}