// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemOwnership.h"
#include "FaerieContainerIterator.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieSubObjectFilter.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"

#include "Fragments/FaerieItemOwningContainer.h"

#include "GameFramework/Actor.h"

namespace Faerie::Container
{
	static int32 BlockTakeOwnershipScopeCount = 0;

	FBlockTakeOwnershipScope::FBlockTakeOwnershipScope() { BlockTakeOwnershipScopeCount++; }
	FBlockTakeOwnershipScope::~FBlockTakeOwnershipScope() { BlockTakeOwnershipScopeCount--; }

	bool ValidateItemData(const FFaerieItemInstance& Instance)
	{
		if (Instance.IsEmpty())
		{
			return false;
		}

		if (Instance.HasItemAsset())
		{
			const UFaerieItem* Asset = Instance.GetItemPtr();
			// Check that the item version is correct.
			if (Asset->GetAssetFormatVersion() < static_cast<int32>(ItemData::EFormatVersion::LatestVersion))
			{
				// @todo write conversions handlers to upgrade versions
				return false;
			}
		}

		if (Instance.HasMassEntity())
		{
			const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
			// @Todo entity validation
		}

		return true;
	}


	FMassEntityHandle GetContainerOwner(const FMassEntityManager& EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container)
	{
		if (const FNestedContainer* NestedContainer = Container->ReadContainerData<FNestedContainer>(EntityManager))
		{
			UE_CLOGF(!NestedContainer->ItemHandle.IsValid(), LogFaerieInventory, Error, "GetContainerOwner found NestedContainer, but ItemHandle was not valid!")
			UE_CLOGF(!EntityManager.IsEntityActive(NestedContainer->ItemHandle), LogFaerieInventory, Error, "GetContainerOwner found NestedContainer, but ItemHandle was not built!")
			return NestedContainer->ItemHandle;
		}

		// Container has no owner.
		return FMassEntityHandle();
	}

	UFaerieItemContainerBase* GetItemOwner(const FMassEntityManager& EntityManager, const FMassEntityHandle Item)
	{
		if (!EntityManager.IsEntityValid(Item) || !EntityManager.IsEntityBuilt(Item))
		{
			UE_LOGF(LogFaerieInventory, Warning, "GetItemOwner called with invalid Item!")
			return nullptr;
		}

		if (const FFaerieMassItemOwningContainer* Fragment = EntityManager.GetConstSharedFragmentDataPtr<FFaerieMassItemOwningContainer>(Item))
		{
			UE_CLOGF(!IsValid(Fragment->Owner), LogFaerieInventory, Error, "GetItemOwner found MassItemOwningContainer, but Owner ptr was not valid!")
			return Fragment->Owner;
		}

		// Item has no owner.
		return nullptr;
	}

	UFaerieItemContainerBase* GetItemOwner(const FFaerieItemProxy& Proxy)
	{
		const TSharedPtr<FMassEntityManager> EntityManager = Proxy.GetEntityManager();
		if (!EntityManager.IsValid())
		{
			return nullptr;
		}
		return GetItemOwner(*EntityManager.Get(), Proxy.GetEntityHandle());
	}

	namespace
	{
		template <bool IsSubItem>
		void ReleaseOwnership_Impl(FMassEntityManager& EntityManager, AActor* RegisteringActor, const TNotNull<UFaerieItemContainerBase*> Owner, const FMassEntityHandle Item)
		{
			TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
			SubObject::GetContainersInInstanceDirect<UFaerieItemContainerBase>(EntityManager, Item, Containers);
			for (TNotNull<UFaerieItemContainerBase*> Container : Containers)
			{
				if (RegisteringActor)
				{
					Container->DeinitializeContainer(EntityManager);
					RegisteringActor->RemoveReplicatedSubObject(Container);
				}

				// If the object has an extension parent, clear it.
				Container->ClearParentExtensions();

				check(Container->GetOuter() == Owner);
				Container->Rename(nullptr, GetTransientPackageAsObject());

				// If the object contains nested items, release ownership recursively.
				for (auto It = Container::MutableItemRange(Container); It; ++It)
				{
					ReleaseOwnership_Impl<true>(EntityManager, RegisteringActor, Owner, It.GetEntityHandle());
				}
			}

			if constexpr (!IsSubItem)
			{
				// Remove the owner fragment
				if (EntityManager.IsProcessing())
				{
					EntityManager.Defer().PushCommand<FMassDeferredRemoveCommand>([Item](FMassEntityManager& InEntityManager)
					{
						if (InEntityManager.IsEntityValid(Item))
						{
							InEntityManager.RemoveConstSharedFragmentFromEntity<FFaerieMassItemOwningContainer>(Item);
						}
					});
				}
				else
				{
					EntityManager.RemoveConstSharedFragmentFromEntity<FFaerieMassItemOwningContainer>(Item);
				}
			}
		}

		template <bool IsSubItem>
		void TakeOwnership_Impl(FMassEntityManager& EntityManager, const TNotNull<AActor*> RegisteringActor, const TNotNull<UFaerieItemContainerBase*> Owner, const FMassEntityHandle Item)
		{
			/*
			if (EntityManager.IsEntityValid(Entity) && EntityManager.IsEntityBuilt(Entity))
			{
				checkfSlow(!EntityManager.GetConstSharedFragmentDataPtr<FFaerieMassItemOwningContainer>(Entity), TEXT("This should always have been removed by the previous owner!"))
			}
			else
			{
				Instance.InitializeMassEntity(EntityManager);
				Entity = Instance.GetMassEntityHandle();
			}
			*/

			// Make sure we are already build and do not already have an owner bound to this item.
			check(EntityManager.IsEntityValid(Item) && EntityManager.IsEntityBuilt(Item))
			checkfSlow(!EntityManager.GetConstSharedFragmentDataPtr<FFaerieMassItemOwningContainer>(Item), TEXT("This should always have been removed by the previous owner!"))

			// Mark the instance with an owner fragment.
			const FConstSharedStruct SharedOwner = EntityManager.GetOrCreateConstSharedFragment<FFaerieMassItemOwningContainer>(Owner);
			if (EntityManager.IsProcessing())
			{
				EntityManager.Defer().PushCommand<FMassDeferredAddCommand>([Item, SharedOwner](FMassEntityManager& InEntityManager)
					{
						if (InEntityManager.IsEntityValid(Item))
						{
							InEntityManager.AddConstSharedFragmentToEntity(Item, SharedOwner);
						}
					});
			}
			else
			{
				EntityManager.AddConstSharedFragmentToEntity(Item, SharedOwner);
			}

			// Perform subitem logic.
			TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
			SubObject::GetContainersInInstanceDirect<UFaerieItemContainerBase>(EntityManager, Item, Containers);
			for (TNotNull<UFaerieItemContainerBase*> Container : Containers)
			{
				UE_LOGF(LogFaerieInventory, Log, "Sub-container (%ls) found. Recursing TakeOwnership!", *Container->GetName())

				if (Container->GetOuter() != Owner)
				{
					check(Container->GetOuter() == GetTransientPackageAsObject());
					Container->Rename(nullptr, Owner);
				}

				RegisteringActor->AddReplicatedSubObject(Container);
				Container->InitializeContainer(EntityManager);

				Container->SetParentExtensions(Owner);

				// If the object contains nested items, take ownership recursively.
				for (auto It = Container::MutableItemRange(Container); It; ++It)
				{
					TakeOwnership_Impl<true>(EntityManager, RegisteringActor, Container, It.GetEntityHandle());
				}
			}
		}
	}

	void ClearOwnership(FMassEntityManager& EntityManager, const FMassEntityHandle Item)
	{
		if (UFaerieItemContainerBase* Outer = GetItemOwner(EntityManager, Item))
		{
			ReleaseOwnership(EntityManager, Outer, Item);
		}
	}

	void ReleaseOwnership(FMassEntityManager& EntityManager, const TNotNull<UFaerieItemContainerBase*> Owner, const FMassEntityHandle Item)
	{
		AActor* RegisteringActor = [Owner]() -> AActor*
		{
			if (AActor* Actor = Owner->GetTypedOuter<AActor>();
				IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
			{
				return Actor;
			}
			return nullptr;
		}();

		UE_CLOGF(!IsValid(RegisteringActor), LogFaerieInventory, Warning, "Unable to find owning Actor for container. Release ownership will continue anyway!")

		ReleaseOwnership_Impl<false>(EntityManager, RegisteringActor, Owner, Item);
	}

	void TakeOwnership(FMassEntityManager& EntityManager, const TNotNull<UFaerieItemContainerBase*> Owner, const FMassEntityHandle Instance)
	{
		if (BlockTakeOwnershipScopeCount > 0) return;

		AActor* RegisteringActor = [Owner]() -> AActor*
		{
			if (AActor* Actor = Owner->GetTypedOuter<AActor>();
				IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
			{
				return Actor;
			}
			return nullptr;
		}();

		if (!ensure(IsValid(RegisteringActor)))
		{
			UE_LOGF(LogFaerieInventory, Error, "Unable to find owning Actor for container (%ls). Unable to take ownership of item instance!", *Owner->GetName())
			return;
		}

		TakeOwnership_Impl<false>(EntityManager, RegisteringActor, Owner, Instance);
	}
}
