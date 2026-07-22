// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemOwnership.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerIterator.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieSubObjectFilter.h"
#include "ItemContainerExtensionBase.h"

#include "GameFramework/Actor.h"

namespace Faerie::Container
{
	bool ValidateItemData(const ItemData::FReference& Reference)
	{
		bool HitError = false;

		if (Reference->HasItemAsset())
		{
			const UFaerieItem* Asset = Reference->GetItemPtr();
			// Check that the item version is correct.
			if (Asset->GetAssetFormatVersion() < static_cast<int32>(ItemData::EFormatVersion::LatestVersion))
			{
				// @todo write conversions handlers to upgrade versions
				return false;
			}
		}

		if (Reference->HasMassEntity())
		{
			const auto Entity = Reference->GetMassEntityHandle();
			// @Todo entity validation
		}

		return !HitError;
	}

	UFaerieItemContainerBase* GetItemOwner(const ItemData::FRequireEntityManager& EntityManager, const ItemData::FMutableReference& Instance)
	{
		const FMassEntityHandle Entity = Instance->GetMassEntityHandle();
		if (EntityManager->IsEntityValid(Entity))
		{
			if (const FFaerieMassItemOwner* Fragment = EntityManager->GetConstSharedFragmentDataPtr<FFaerieMassItemOwner>(Entity))
			{
				return Cast<UFaerieItemContainerBase>(Fragment->GetInterface());
			}
		}
		return nullptr;
	}

	template <bool IsSubItem>
	void ReleaseOwnership_Impl(const ItemData::FRequireEntityManager& EntityManager, AActor* RegisteringActor, const TNotNull<UFaerieItemContainerBase*> Owner, const ItemData::FMutableReference& Instance)
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
				ReleaseOwnership_Impl<true>(EntityManager, RegisteringActor, Owner, It.GetReference());
			}
		}

		if constexpr (!IsSubItem)
		{
			// Remove the owner fragment
			EntityManager->RemoveConstSharedFragmentFromEntity<FFaerieMassItemOwner>(Instance->GetMassEntityHandle());
		}
	}

	void ClearOwnership(const ItemData::FRequireEntityManager& EntityManager, const ItemData::FMutableReference& Instance)
	{
		if (UFaerieItemContainerBase* Outer = GetItemOwner(EntityManager, Instance))
		{
			ReleaseOwnership(EntityManager, Outer, Instance);
		}
	}

	template <bool IsSubItem>
	void TakeOwnership_Impl(const ItemData::FRequireEntityManager& EntityManager, AActor* RegisteringActor, const TNotNull<UFaerieItemContainerBase*> Owner, const ItemData::FMutableReference& Instance)
	{
		// If the item is not a SubItem, mark it with an owner fragment.
		if constexpr (!IsSubItem)
		{
			// Make sure we do not already have an owner bound to this instance.
			if (EntityManager->IsEntityValid(Instance->GetMassEntityHandle()))
			{
				checkfSlow(!EntityManager->GetConstSharedFragmentDataPtr<FFaerieMassItemOwner>(Instance->GetMassEntityHandle()), TEXT("This should always have been removed by the previous owner!"))
			}

			Instance->InitializeMassEntityIfInvalid(EntityManager);

			const FConstSharedStruct SharedOwner = EntityManager->GetOrCreateConstSharedFragment<FFaerieMassItemOwner>(Owner);
			EntityManager->AddConstSharedFragmentToEntity(Instance->GetMassEntityHandle(), SharedOwner);
		}

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
				TakeOwnership_Impl<true>(EntityManager, RegisteringActor, Owner, It.GetReference());
			}
		}
	}

	void ReleaseOwnership(const ItemData::FRequireEntityManager& EntityManager, const TNotNull<UFaerieItemContainerBase*> Owner, const ItemData::FMutableReference& Instance)
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

	void TakeOwnership(const ItemData::FRequireEntityManager& EntityManager, const TNotNull<UFaerieItemContainerBase*> Owner, const ItemData::FMutableReference& Instance)
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

		TakeOwnership_Impl<false>(EntityManager, RegisteringActor, Owner, Instance);
	}
}
