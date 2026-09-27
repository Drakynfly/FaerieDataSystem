// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ViewModels/FaerieMassEntityViewModelBase.h"
#include "EntityManagerHelpers.h"
#include "MassEntityManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassEntityViewModelBase)

using namespace Faerie;

void UFaerieMassEntityViewModelBase::BeginDestroy()
{
	if (EntityHandle.IsValid())
	{
		UWorld* World = GetWorld();
		if (World && ItemData::HasFaerieEntityManagerBeenAssigned(World))
		{
			FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);
			EntityManager.DestroyEntity(EntityHandle);
		}
	}

	Super::BeginDestroy();
}

void UFaerieMassEntityViewModelBase::CreateViewModelEntity(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType)
{
	FInstancedStruct EntityStruct;
	EntityStruct.InitializeAs(FragmentType);
	EntityStruct.GetMutable<Container::FViewModelFragment>().ViewObject = this;
	EntityHandle = EntityManager.CreateEntity(MakeConstArrayView(&EntityStruct, 1));
}

void UFaerieMassEntityViewModelBase::DestroyViewModelEntity(FMassEntityManager& EntityManager)
{
	if (EntityManager.IsEntityValid(EntityHandle))
	{
		EntityManager.DestroyEntity(EntityHandle);
		EntityHandle.Reset();
	}
}
