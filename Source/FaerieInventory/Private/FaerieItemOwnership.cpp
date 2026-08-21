// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemOwnership.h"
#include "FaerieContainerIterator.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieSubObjectFilter.h"
#include "ItemContainerExtensionBase.h"

#include "GameFramework/Actor.h"

namespace Faerie::Container
{
	bool ValidateItemData(const FFaerieItemInstance& Instance)
	{
		bool HitError = false;

		if (Instance.HasItemAsset())
		{
			auto Asset = Instance.GetItemPtr();
			// Check that the item version is correct.
			if (Asset->GetAssetFormatVersion() < static_cast<int32>(ItemData::EFormatVersion::LatestVersion))
			{
				// @todo write conversions handlers to upgrade versions
				return false;
			}
		}

		if (Instance.HasMassEntity())
		{
			const auto Entity = Instance.GetMassEntityHandle();
			// @Todo entity validation
		}

		return !HitError;
	}

	UFaerieItemContainerBase* GetItemOwner(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Instance)
	{
		const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
		if (EntityManager.IsEntityValid(Entity))
		{
			if (const FFaerieMassItemOwner* Fragment = EntityManager.GetConstSharedFragmentDataPtr<FFaerieMassItemOwner>(Entity))
			{
				return Cast<UFaerieItemContainerBase>(Fragment->GetInterface());
			}
		}
		return nullptr;
	}

	namespace
	{
		template <bool IsSubItem>
		void ReleaseOwnership_Impl(FMassEntityManager& EntityManager, AActor* RegisteringActor, const TNotNull<UFaerieItemContainerBase*> Owner, const FFaerieItemInstance& Instance)
		{
			TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
			SubObject::GetContainersInInstanceDirect(EntityManager, Instance, Containers);
			for (TNotNull<UFaerieItemContainerBase*> Container : Containers)
			{
				if (RegisteringActor)
				{
					Container->DeinitializeNetObject(RegisteringActor);
					RegisteringActor->RemoveReplicatedSubObject(Container);
				}

				// If the object has an extension group, clear its parent.
				Container->GetExtensions()->ClearParentGroup();

				// If the object contains nested items, release ownership recursively.
				for (auto It = Container::MutableItemRange(Container); It; ++It)
				{
					ReleaseOwnership_Impl<true>(EntityManager, RegisteringActor, Owner, It.GetInstance());
				}
			}

			if constexpr (!IsSubItem)
			{
				// Remove the owner fragment
				EntityManager.RemoveConstSharedFragmentFromEntity<FFaerieMassItemOwner>(Instance.GetMassEntityHandle());
			}
		}

		void TakeOwnership_Impl_SubItem(FMassEntityManager& EntityManager, AActor* RegisteringActor, const TNotNull<UFaerieItemContainerBase*> Owner, const FFaerieItemInstance& Instance)
		{
			UItemContainerExtensionGroup* OuterExtensions = Owner->GetExtensions();

			TArray<TNotNull<UFaerieItemContainerBase*>> Containers;
			SubObject::GetContainersInInstanceDirect(EntityManager, Instance, Containers);
			for (TNotNull<UFaerieItemContainerBase*> Container : Containers)
			{
				if (RegisteringActor)
				{
					RegisteringActor->AddReplicatedSubObject(Container);
					Container->InitializeNetObject(RegisteringActor);
				}

				// If the object has an extension group, set its parent to ours.
				if (IsValid(OuterExtensions))
				{
					Container->GetExtensions()->SetParentGroup(OuterExtensions);
				}

				// If the object contains nested items, take ownership recursively.
				for (auto It = Container::MutableItemRange(Container); It; ++It)
				{
					TakeOwnership_Impl_SubItem(EntityManager, RegisteringActor, Owner, It.GetInstance());
				}
			}
		}

		void TakeOwnership_Impl_Root(FMassEntityManager& EntityManager, AActor* RegisteringActor, const TNotNull<UFaerieItemContainerBase*> Owner, FFaerieItemInstance& Instance)
		{
			FMassEntityHandle Entity = Instance.GetMassEntityHandle();

			// Make sure we do not already have an owner bound to this instance.
			if (EntityManager.IsEntityValid(Entity))
			{
				checkfSlow(!EntityManager.GetConstSharedFragmentDataPtr<FFaerieMassItemOwner>(Entity), TEXT("This should always have been removed by the previous owner!"))
			}
			else
			{
				Instance.InitializeMassEntity(EntityManager);
				Entity = Instance.GetMassEntityHandle();
			}

			// Mark the instance with an owner fragment.
			const FConstSharedStruct SharedOwner = EntityManager.GetOrCreateConstSharedFragment<FFaerieMassItemOwner>(Owner);
			EntityManager.AddConstSharedFragmentToEntity(Entity, SharedOwner);

			// Also perform subitem logic.
			TakeOwnership_Impl_SubItem(EntityManager, RegisteringActor, Owner, Instance);
		}
	}

	void ClearOwnership(FMassEntityManager& EntityManager, const FFaerieItemInstance& Instance)
	{
		if (UFaerieItemContainerBase* Outer = GetItemOwner(EntityManager, Instance))
		{
			ReleaseOwnership(EntityManager, Outer, Instance);
		}
	}

	void ReleaseOwnership(FMassEntityManager& EntityManager, const TNotNull<UFaerieItemContainerBase*> Owner, const FFaerieItemInstance& Instance)
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

		ReleaseOwnership_Impl<false>(EntityManager, RegisteringActor, Owner, Instance);
	}

	void TakeOwnership(FMassEntityManager& EntityManager, const TNotNull<UFaerieItemContainerBase*> Owner, FFaerieItemInstance& Instance)
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

		TakeOwnership_Impl_Root(EntityManager, RegisteringActor, Owner, Instance);
	}
}
