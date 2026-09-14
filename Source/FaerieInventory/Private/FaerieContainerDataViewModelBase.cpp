// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerDataViewModelBase.h"
#include "EntityManagerHelpers.h"
#include "MassCommandBuffer.h"
#include "MassEntityManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerDataViewModelBase)

using namespace Faerie;

void UFaerieMassEntityViewModelBase::BeginDestroy()
{
	if (FMassEntityManager* EntityManager = ItemData::GetFaerieEntityManager())
	{
		if (EntityHandle.IsValid())
		{
			EntityManager->DestroyEntity(EntityHandle);
		}
	}

	Super::BeginDestroy();
}

void UFaerieMassEntityViewModelBase::InitEntity(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType)
{
	FInstancedStruct EntityStruct;
	EntityStruct.InitializeAs(FragmentType);
	EntityStruct.GetMutable<Container::FViewModelFragment>().ViewObject = this;
	EntityHandle = EntityManager.CreateEntity(MakeConstArrayView(&EntityStruct, 1));
}

void UFaerieContainerDataViewModelBase::InitializeView(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType,
													   const TPair<FWeakObjectPtr, FFaerieItemContainerExtensions*>& ExtensionPtrTemp)
{
	ContainerExtensionPtr = ExtensionPtrTemp;

	if (EntityManager.IsProcessing())
	{
		EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
			[WeakThis = TWeakObjectPtr<ThisClass>(this), FragmentType](FMassEntityManager& DeferredEntityManager)
			{
				if (auto This = WeakThis.Get())
				{
					This->InitEntity(DeferredEntityManager, FragmentType);
					This->SyncView();
				}
			});
	}
	else
	{
		InitEntity(EntityManager, FragmentType);
		SyncView();
	}
}
