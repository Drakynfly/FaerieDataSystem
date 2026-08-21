// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieMassReplicationActor.h"
#include "MassReplication/FaerieViewModelSubsystem.h"

#include "FaerieItem.h"
#include "EntityManagerHelpers.h"
#include "MassCommandBuffer.h"

#include "Engine/Engine.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassReplicationActor)

using namespace Faerie;

void FFaerieMassReplicatedEntity::PreReplicatedRemove(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOG(LogTemp, Verbose, TEXT("FFaerieMassReplicatedEntity::PreReplicatedRemove"))
	InArraySerializer.Owner->Client_RemoveEntity(*this);
}

void FFaerieMassReplicatedEntity::PostReplicatedAdd(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOG(LogTemp, Verbose, TEXT("FFaerieMassReplicatedEntity::PostReplicatedAdd"))
	InArraySerializer.Owner->Client_AddEntity(*this);
}

void FFaerieMassReplicatedEntity::PostReplicatedChange(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOG(LogTemp, Verbose, TEXT("FFaerieMassReplicatedEntity::PostReplicatedChange"))
	InArraySerializer.Owner->Client_UpdateEntity(*this);
}

AFaerieMassReplicationActor::AFaerieMassReplicationActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	ReplicatedEntities.Owner = this;
}

void AFaerieMassReplicationActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedEntities, SharedParams);
}

void AFaerieMassReplicationActor::BeginPlay()
{
	Super::BeginPlay();

	const UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::Assert);
	ViewModelSubsystem = World->GetSubsystemChecked<UFaerieViewModelSubsystem>();
}

void AFaerieMassReplicationActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Client-side check to fixup mass after receiving item pointers.
	if (GetNetMode() == NM_Client)
	{
		for (auto&& Entity : ReplicatedEntities.Entries)
		{
			Client_CheckItemPointer(Entity);
		}
	}
}

void AFaerieMassReplicationActor::Server_UpdateFragment(const FFaerieItemInstance& Item, const TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews)
{
	const FMassEntityHandle Entity = Item.GetMassEntityHandle();

	// Try to update existing entry.
	for (auto&& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		if (ReplicatedEntity.EntityHandle != Entity) continue;

		for (auto&& FragmentView : FragmentViews)
		{
			// Linear search. Probably will not have enough fragments in one item for this to be slow enough to attempt optimizing.
			if (auto* ReplicatedFragment = ReplicatedEntity.Fragments.FindByPredicate(
				[&FragmentView](const FInstancedStruct& Fragment)
				{
					return Fragment.GetScriptStruct() == FragmentView.GetScriptStruct();
				}))
			{
				// Found the entity, overwrite existing fragment
				(*ReplicatedFragment) = FragmentView;
			}
			else
			{
				// There was no existing fragment for this entity.
				ReplicatedEntity.Fragments.Emplace(FragmentView);
			}
		}

		ReplicatedEntities.MarkItemDirty(ReplicatedEntity);
		return;
	}

	// There was no existing entry for this entity.
	FFaerieMassReplicatedEntity& NewEntry = ReplicatedEntities.Entries.AddDefaulted_GetRef();
	NewEntry.EntityHandle = Entity;
	for (auto&& FragmentView : FragmentViews)
	{
		NewEntry.Fragments.Emplace(FragmentView);
	}
	ReplicatedEntities.MarkItemDirty(NewEntry);
}

void AFaerieMassReplicationActor::Server_RemoveFragment(const FFaerieItemInstance& Item, const TNotNull<const UScriptStruct*> ScriptStruct)
{
	const FMassEntityHandle Entity = Item.GetMassEntityHandle();
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	if (!EntityManager.IsEntityValid(Entity))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Item created without entity handle. Item creation should always initialize itself with mass!"))
		return;
	}

	for (auto&& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		if (ReplicatedEntity.EntityHandle != Entity) continue;

		for (auto&& ReplicatedFragment : ReplicatedEntity.Fragments)
		{
			if (ReplicatedFragment.GetScriptStruct() == ScriptStruct)
			{
				EntityManager.RemoveFragmentFromEntity(Entity, ScriptStruct);
				ReplicatedEntities.MarkArrayDirty();
				return;
			}
		}
	}
}

void AFaerieMassReplicationActor::Server_RemoveEntity(const FFaerieItemInstance& Item)
{
	const FMassEntityHandle Entity = Item.GetMassEntityHandle();
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	if (!EntityManager.IsEntityValid(Entity))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Item created without entity handle. Item creation should always initialize itself with mass!"))
		return;
	}

	for (auto&& It = ReplicatedEntities.Entries.CreateIterator(); It; ++It)
	{
		const FFaerieMassReplicatedEntity& ReplicatedEntity = *It;
		if (ReplicatedEntity.EntityHandle == Entity)
		{
			It.RemoveCurrent();
			ReplicatedEntities.MarkArrayDirty();
			return;
		}
	}
}

void AFaerieMassReplicationActor::Client_AddEntity(FFaerieMassReplicatedEntity& Entity)
{
	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	const FFaerieItemInstance TempInstance = FFaerieItemInstance::FromFragments(EntityManager, Entity.Fragments);
	Entity.EntityHandle = TempInstance.GetMassEntityHandle();
	for (auto&& Fragment : Entity.Fragments)
	{
		ViewModelSubsystem->Client_PostReplicationChange(TempInstance, Fragment);
	}

	Client_CheckItemPointer(Entity);
}

void AFaerieMassReplicationActor::Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity)
{
	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	// @todo figure out what changed and only broadcast for them

	FFaerieItemInstance TempInstance;

	// Local item has already been initialized, apply delta.
	TempInstance.DestroyMassEntity(EntityManager);

	TempInstance.ImportFragmentData(EntityManager, Entity.Fragments);
	Entity.EntityHandle = TempInstance.GetMassEntityHandle();

	for (auto&& Fragment : Entity.Fragments)
	{
		ViewModelSubsystem->Client_PostReplicationChange(TempInstance, Fragment);
	}
}

void AFaerieMassReplicationActor::Client_RemoveEntity(const FFaerieMassReplicatedEntity& Entity)
{
	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	EntityManager.DestroyEntity(Entity.EntityHandle);
}

void AFaerieMassReplicationActor::Client_CheckItemPointer(FFaerieMassReplicatedEntity& Entity)
{
	if (!Entity.HasImportedItemPointer && IsValid(Entity.ItemPointer))
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();

		// If the entity manager stores info for this instance, push our item to it.
		if (EntityManager.IsEntityValid(Entity.EntityHandle))
		{
			EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
					[Handle = Entity.EntityHandle, Item = Entity.ItemPointer](FMassEntityManager& InEntityManager)
					{
						// All faerie item instances must have this fragment.
						FFaerieMassItemPointer& FragmentPtr = InEntityManager.GetFragmentDataChecked<FFaerieMassItemPointer>(Handle);
						FragmentPtr.Item = Item;
					});
		}
		Entity.HasImportedItemPointer = true;
	}
}
