// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerBase.h"

#include "UObject/WeakInterfacePtr.h"

#include "ViewModels/FaerieMassEntityViewModelBase.h"

#include "FaerieContainerDataViewModelBase.generated.h"

#define FAE_API FAERIEINVENTORY_API


/**
 * Base class for View Model objects that reflect Faerie Container data.
 */
UCLASS(Abstract)
class FAE_API UFaerieContainerDataViewModelBase : public UFaerieMassEntityViewModelBase
{
	GENERATED_BODY()

public:
	// Call to set up this view. The entity will be created, and then SyncView will be called.
	void InitializeView(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType, TNotNull<IFaerieTempInterfaceForGettingParentExtensions*> ExtensionPtrTemp);

	virtual void SyncView(FMassEntityManager& EntityManager) {}

	UObject* GetContainerObject() const { return ExtensionPtr.GetObject(); }

protected:
	// The extension data that we view. Currently stored via interface until refactor of equipment manager.
	TWeakInterfacePtr<IFaerieTempInterfaceForGettingParentExtensions> ExtensionPtr;
};

#undef FAE_API
