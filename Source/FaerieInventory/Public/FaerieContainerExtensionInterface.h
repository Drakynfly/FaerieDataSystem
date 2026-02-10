// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryConcepts.h"
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
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const
		PURE_VIRTUAL(IFaerieContainerExtensionInterface::GetExtensionGroup, return nullptr; )

	// Has extension by class
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions")
	virtual bool HasExtension(TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch = true) const;

	// Get extension by class
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", meta = (DeterminesOutputType = ExtensionClass))
	virtual UItemContainerExtensionBase* GetExtension(UPARAM(meta = (AllowAbstract = "false")) TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch) const;

	/*
	// Doesn't compile for some reason
	template <CItemContainerExtension T>
	T* GetExtension() const
	{
		return Cast<T>(GetExtension(T::StaticClass()));
	}
	*/

	// Try to add an extension to this storage. This will only fail if the extension pointer is invalid or already added.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions")
	virtual bool AddExtension(UItemContainerExtensionBase* Extension);

	// Add a new extension of the given class, and return the result. If an extension of this class already exists, it
	// will be returned instead.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", meta = (DeterminesOutputType = "ExtensionClass"), DisplayName = "Add Extension (by class)")
	virtual UItemContainerExtensionBase* AddExtensionByClass(TSubclassOf<UItemContainerExtensionBase> ExtensionClass);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions")
	virtual bool RemoveExtension(UItemContainerExtensionBase* Extension);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", DisplayName = "Remove Extension (by class)")
	virtual bool RemoveExtensionByClass(TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch = true);

protected:
	/*
     * Get an extension of a certain class.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|Extensions",
    	meta = (DeterminesOutputType = ExtensionClass, DynamicOutputParam = Extension, ExpandBoolAsExecs = "ReturnValue"))
    virtual bool GetExtensionChecked(UPARAM(meta = (AllowAbstract = "false")) TSubclassOf<UItemContainerExtensionBase> ExtensionClass,
    	UItemContainerExtensionBase*& Extension, bool RecursiveSearch = true) const;
};

// Outside IFaerieContainerExtensionInterface because it won't compile there
template <Faerie::CItemContainerExtension T>
const T* GetExtension(const IFaerieContainerExtensionInterface* Interface, const bool RecursiveSearch)
{
	return CastChecked<T>(Interface->GetExtension(T::StaticClass(), RecursiveSearch), ECastCheckedType::NullAllowed);
}

template <Faerie::CItemContainerExtension T>
T* GetExtension(IFaerieContainerExtensionInterface* Interface, const bool RecursiveSearch)
{
	return CastChecked<T>(Interface->GetExtension(T::StaticClass(), RecursiveSearch), ECastCheckedType::NullAllowed);
}