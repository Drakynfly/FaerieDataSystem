// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

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
	virtual bool HasExtension(TSubclassOf<UItemContainerExtensionBase> ExtensionClass) const;

	// Get extension by class
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", meta = (DeterminesOutputType = ExtensionClass))
	virtual UItemContainerExtensionBase* GetExtension(UPARAM(meta = (AllowAbstract = "false")) TSubclassOf<UItemContainerExtensionBase> ExtensionClass) const;

	/*
	// Doesn't compile for some reason
	template <
		typename TExtensionClass
		UE_REQUIRES(TIsDerivedFrom<TExtensionClass, UItemContainerExtensionBase>::Value)
	>
	TExtensionClass* GetExtension() const
	{
		return Cast<TExtensionClass>(GetExtension(TExtensionClass::StaticClass()));
	}
	*/

	// Try to add an extension to this storage. This will only fail if the extension pointer is invalid or already added.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", BlueprintAuthorityOnly)
	virtual bool AddExtension(UItemContainerExtensionBase* Extension);

	// Add a new extension of the given class, and return the result. If an extension of this class already exists, it
	// will be returned instead.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", BlueprintAuthorityOnly, meta = (DeterminesOutputType = "ExtensionClass"), DisplayName = "Add Extension (by class)")
	virtual UItemContainerExtensionBase* AddExtensionByClass(TSubclassOf<UItemContainerExtensionBase> ExtensionClass);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", BlueprintAuthorityOnly)
	virtual bool RemoveExtension(UItemContainerExtensionBase* Extension);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Extensions", BlueprintAuthorityOnly, DisplayName = "Remove Extension (by class)")
	virtual bool RemoveExtensionByClass(TSubclassOf<UItemContainerExtensionBase> ExtensionClass);

protected:
	/*
     * Get an extension of a certain class.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|Extensions",
    	meta = (DeterminesOutputType = ExtensionClass, DynamicOutputParam = Extension, ExpandBoolAsExecs = "ReturnValue"))
    virtual bool GetExtensionChecked(UPARAM(meta = (AllowAbstract = "false")) TSubclassOf<UItemContainerExtensionBase> ExtensionClass,
    	UItemContainerExtensionBase*& Extension) const;
};

// Outside IFaerieContainerExtensionInterface because it won't compile there
template <
	typename TExtensionClass
	UE_REQUIRES(TIsDerivedFrom<TExtensionClass, UItemContainerExtensionBase>::Value)
>
const TExtensionClass* GetExtension(const IFaerieContainerExtensionInterface* Interface)
{
	return Cast<TExtensionClass>(Interface->GetExtension(TExtensionClass::StaticClass()));
}

template <
	typename TExtensionClass
	UE_REQUIRES(TIsDerivedFrom<TExtensionClass, UItemContainerExtensionBase>::Value)
>
TExtensionClass* GetExtension(IFaerieContainerExtensionInterface* Interface)
{
	return Cast<TExtensionClass>(Interface->GetExtension(TExtensionClass::StaticClass()));
}