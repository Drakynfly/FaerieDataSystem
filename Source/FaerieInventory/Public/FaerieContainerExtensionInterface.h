// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryConcepts.h"

#include "Templates/SubclassOf.h"

#include "UObject/Interface.h"
#include "FaerieContainerExtensionInterface.generated.h"

class UItemContainerExtensionBase;
class UItemContainerExtensionGroup;

UINTERFACE(NotBlueprintable)
class UFaerieContainerExtensionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class FAERIEINVENTORY_API IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions")
	virtual UItemContainerExtensionGroup* VirtualGetExtensionGroup() const
		PURE_VIRTUAL(IFaerieContainerExtensionInterface::GetExtensionGroup, return nullptr; )

protected:
	// BLUEPRINT USE ONLY

	/*
     * Get an extension of a certain class.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|Extensions",
    	meta = (DeterminesOutputType = ExtensionClass, DynamicOutputParam = Extension, ExpandBoolAsExecs = "ReturnValue"))
    virtual bool GetExtensionChecked(UPARAM(meta = (AllowAbstract = "false")) TSubclassOf<UItemContainerExtensionBase> ExtensionClass,
    	UItemContainerExtensionBase*& Extension, bool RecursiveSearch = true) const;
};

namespace Faerie::Extensions
{
	FAERIEINVENTORY_API const UItemContainerExtensionBase* Get(const UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch);
	FAERIEINVENTORY_API UItemContainerExtensionBase* Get(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch);

	template <Container::CItemContainerExtension T>
	const T* Get(const UItemContainerExtensionGroup* Group, const bool RecursiveSearch)
	{
		return CastChecked<T>(Get(Group, T::StaticClass(), RecursiveSearch), ECastCheckedType::NullAllowed);
	}

	template <Container::CItemContainerExtension T>
	T* Get(UItemContainerExtensionGroup* Group, const bool RecursiveSearch)
	{
		return CastChecked<T>(Get(Group, T::StaticClass(), RecursiveSearch), ECastCheckedType::NullAllowed);
	}

	// Add a new extension of the given class, and return the result. If an extension of this class already exists, it
	// will be returned instead.
	UItemContainerExtensionBase* AddExtensionByClass(UItemContainerExtensionGroup* Group, TSubclassOf<UItemContainerExtensionBase> ExtensionClass);

	bool RemoveExtensionByClass(UItemContainerExtensionGroup* Group, TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch = true);
}