// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerDataViewModelBase.h"
#include "MassCommandBuffer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerDataViewModelBase)

using namespace Faerie;

void UFaerieContainerDataViewModelBase::InitializeView(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType,
													   const TNotNull<IFaerieTempInterfaceForGettingParentExtensions*> ExtensionPtrTemp)
{
	check(GetWorld()) // We need an outer that can fetch a world so we can destroy our entity in BeginDestroy
	ExtensionPtr = ExtensionPtrTemp;

	check(!IsInitialized());
	if (EntityManager.IsProcessing())
	{
		EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
			[WeakThis = TWeakObjectPtr<ThisClass>(this), FragmentType](FMassEntityManager& DeferredEntityManager)
			{
				if (UFaerieContainerDataViewModelBase* This = WeakThis.Get())
				{
					This->CreateViewModelEntity(DeferredEntityManager, FragmentType);
					This->SyncView(DeferredEntityManager);
				}
			});
	}
	else
	{
		CreateViewModelEntity(EntityManager, FragmentType);
		SyncView(EntityManager);
	}
}
