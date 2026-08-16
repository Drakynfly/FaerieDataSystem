// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieMassReplicationActor.h"
#include "MassReplication/FaerieViewModelSubsystem.h"

#include "FaerieItem.h"
#include "MassEntitySubsystem.h"
#include "EntityManagerHelpers.h"

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
	InArraySerializer.Owner->Client_UpdateEntity(*this);
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
	MassEntitySubsystem = World->GetSubsystemChecked<UMassEntitySubsystem>();
	ViewModelSubsystem = World->GetSubsystemChecked<UFaerieViewModelSubsystem>();
}

void AFaerieMassReplicationActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Client-side check to fixup data received before item pointer.
	if (GetNetMode() == NM_Client)
	{
		for (auto&& Entity : ReplicatedEntities.Entries)
		{
			if (Entity.AwaitingItemPointer && Entity.Item.IsValid())
			{
				Client_ProcessUpdateData(Entity);

				Entity.AwaitingItemPointer = false;
			}
		}
	}
}

void AFaerieMassReplicationActor::Server_UpdateFragment(const TValid<const FFaerieItemInstance&> Item, const TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews)
{
	const FMassEntityHandle Entity = ValidGet(Item).GetMassEntityHandle();

	// Try to update existing entry.
	for (auto&& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		const FFaerieItemInstance& Instance = ReplicatedEntity.Item.Get();

		if (Instance.GetMassEntityHandle() != Entity) continue;

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
	NewEntry.Item = ValidGet(Item);
	for (auto&& FragmentView : FragmentViews)
	{
		NewEntry.Fragments.Emplace(FragmentView);
	}
	ReplicatedEntities.MarkItemDirty(NewEntry);
}

void AFaerieMassReplicationActor::Server_RemoveFragment(const TValid<const FFaerieItemInstance&> Item, const TNotNull<const UScriptStruct*> ScriptStruct)
{
	const FMassEntityHandle Entity = ValidGet(Item).GetMassEntityHandle();
	FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
	if (!EntityManager.IsEntityValid(Entity))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Item created without entity handle. Item creation should always initialize itself with mass!"))
		return;
	}

	for (auto&& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		if (ReplicatedEntity.Item.Get() != Item) continue;

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

void AFaerieMassReplicationActor::Server_RemoveEntity(const TValid<const FFaerieItemInstance&> Item)
{
	const FMassEntityHandle Entity = ValidGet(Item).GetMassEntityHandle();
	FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
	if (!EntityManager.IsEntityValid(Entity))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Item created without entity handle. Item creation should always initialize itself with mass!"))
		return;
	}

	for (auto&& It = ReplicatedEntities.Entries.CreateIterator(); It; ++It)
	{
		const FFaerieMassReplicatedEntity& ReplicatedEntity = *It;
		if (ReplicatedEntity.Item.Get() == Item)
		{
			It.RemoveCurrent();
			ReplicatedEntities.MarkArrayDirty();
			return;
		}
	}
}

void AFaerieMassReplicationActor::Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity)
{
	if (Entity.Item.IsValid() && IsValid(MassEntitySubsystem))
	{
		Client_ProcessUpdateData(Entity);
	}
	else
	{
		Entity.AwaitingItemPointer = true;
	}
}

void AFaerieMassReplicationActor::Client_RemoveEntity(FFaerieMassReplicatedEntity& Entity)
{
	if (Entity.Item.IsValid())
	{
		FFaerieItemInstance& Instance = Entity.Item.Get();
		Instance.DestroyMassEntity(MassEntitySubsystem->GetMutableEntityManager());
	}
	else
	{
		Entity.AwaitingItemPointer = true;
	}
}

void AFaerieMassReplicationActor::Client_ProcessUpdateData(FFaerieMassReplicatedEntity& Entity)
{
	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	FFaerieItemInstance& Instance = Entity.Item.Get();

	if (Instance.GetMassEntityHandle().IsValid())
	{
		// @todo figure out what changed and only broadcast for them

		// Local item has already been initialized, apply delta.
		Instance.DestroyMassEntity(EntityManager);

		Instance.ImportFragmentData(EntityManager, Entity.Fragments);

		for (auto&& Fragment : Entity.Fragments)
		{
			ViewModelSubsystem->Client_PostReplicationChange(Instance, Fragment);
		}
	}
	else
	{
		// Local item does not exist in mass, initialize.
		Instance.ImportFragmentData(EntityManager, Entity.Fragments);
		for (auto&& Fragment : Entity.Fragments)
		{
			ViewModelSubsystem->Client_PostReplicationChange(Instance, Fragment);
		}
	}
}
