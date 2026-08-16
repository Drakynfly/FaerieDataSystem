// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemInstance.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"
#include "FaerieItemDataLog.h"
#include "FaerieMassFragment.h"
#include "FaerieItemOwnerInterface.h"
#include "MassEntityBuilder.h"
#include "MassEntityTemplate.h"

#include "Engine/World.h"

#include "MassReplication/FaerieMassReplicationSubsystem.h"
#include "MassReplication/FaerieViewModelSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemInstance)

using namespace Faerie;

void FFaerieItemInstance::InitializeMassEntityImpl(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	const FMassEntityTemplate& EntityTemplate = EntityManager.GetWorld()->GetSubsystemChecked<UFaerieMassReplicationSubsystem>()->GetItemDataTemplate();
	UE::Mass::FEntityBuilder Builder = EntityTemplate.CreateEntityBuilder(EntityManager.AsShared())
		.Add<FFaerieMassItemPointer>(Item) // Add a MassItemPointer struct to label the entity as a faerie item whether there is a valid item pointer or not.
		.Add<FFaerieItemModificationDate>(FDateTime::UtcNow());

	// Look for fragments that need to always be moved into the runtime entity on creation
	for (const FInstancedStruct& DefaultFragment : Item->GetFragmentDefaults())
	{
		const ItemData::FMassFragmentTypeInterface* Traits = ItemData::GetFragmentTraitsInterface(DefaultFragment.GetScriptStruct());
		if (Traits && Traits->HasInitializeRuntime)
		{
			FInstancedStruct FragmentCopy = DefaultFragment;
			// @todo we might want to do a pass the rename fragment subobjects when we are possessed...
			if (Traits->InitializeRuntime(FragmentCopy.GetMutableMemory(), GetTransientPackage(), *this))
			{
				Builder.Add(MoveTemp(FragmentCopy));
			}
		}
	}

	// Add any initial fragments to the builder
	for (auto&& FragmentInstance : Fragments)
	{
		Builder.Add(MoveTemp(FragmentInstance));
	}

	// Assign mass entity handle ahead of calling Commit, because observers can be directly triggered by Commit,
	// and they may want this item to already have an entity handle.
	EntityHandle = Builder.GetEntityHandle();
	Builder.Commit();
}

void FFaerieItemInstance::UpdateTimestamp(bool CreateIfMissing) const
{
	// Update timestamp should only be called on instances that already have a mass equivalence
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	EntityManager.CheckIfEntityIsValid(EntityHandle);

	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[Handle = EntityHandle, CreateIfMissing](FMassEntityManager& InEntityManager)
		{
			if (FFaerieItemModificationDate* FragmentPtr = InEntityManager.GetFragmentDataPtr<FFaerieItemModificationDate>(Handle))
			{
				// Assign new value
				FragmentPtr->LastModified = FDateTime::UtcNow();
			}
			else
			{
				if (CreateIfMissing)
				{
					InEntityManager.AddFragmentToEntity(Handle, FFaerieItemModificationDate::StaticStruct(),
						[](void* Fragment, const UScriptStruct&)
						{
							static_cast<FFaerieItemModificationDate*>(Fragment)->LastModified = FDateTime::UtcNow();
						});
				}
			}
		});
}

void FFaerieItemInstance::NotifyOwnerOfChange(const FMassEntityManager& EntityManager, const TNotNull<const UScriptStruct*> FragmentType, const FGameplayTag Tag) const
{
	if (const FFaerieMassItemOwner* OwnerFragment = EntityManager.GetConstSharedFragmentDataPtr<FFaerieMassItemOwner>(EntityHandle))
	{
		OwnerFragment->GetInterface()->OnItemDataChanged(*this, FragmentType, Tag);
	}
}

bool FFaerieItemInstance::IsMutable() const
{
	if (Item)
	{
		// If the ItemAsset disallows mutation, then we are not mutable.
		if (!Item->CanMutate())
		{
			return false;
		}
	}

	// Otherwise MassEntity based instances are always mutable.
	return true;
}

bool FFaerieItemInstance::UEOpEquals(const FFaerieItemInstance& Other) const
{
	return Item == Other.Item && EntityHandle == Other.EntityHandle;
}

void FFaerieItemInstance::InitializeMassEntity(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	// Only mutable instances are allowed to create a mass entity.
	check(IsMutable())

	// Prevent double-registration!
	check(!EntityManager.IsEntityValid(EntityHandle));

	InitializeMassEntityImpl(EntityManager, Fragments);
}

void FFaerieItemInstance::InitializeMassEntityIfInvalid(FMassEntityManager& EntityManager)
{
	if (IsMutable() && !EntityManager.IsEntityValid(EntityHandle))
	{
		InitializeMassEntityImpl(EntityManager, {});
	}
}

void FFaerieItemInstance::DestroyMassEntity(FMassEntityManager& EntityManager)
{
	if (EntityManager.IsEntityValid(EntityHandle))
	{
		// @Todo
		//EntityManager->GetWorld()->GetSubsystemChecked<UFaerieViewModelSubsystem>()->HandleInstanceDestruction(*this);
		EntityManager.GetWorld()->GetSubsystemChecked<UFaerieMassReplicationSubsystem>()->Server_RemoveEntity(*this);
		EntityManager.DestroyEntity(EntityHandle);
	}
	EntityHandle.Reset();
}

void FFaerieItemInstance::ImportFragmentData(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	if (Fragments.IsEmpty()) return;

	if (!IsMutable())
	{
		UE_LOG(LogFaerieItemData, Error, TEXT("Attempted importing mass instanced to immutable item instance!"))
		return;
	}

	// Prevent double-registration!
	check(!EntityManager.IsEntityValid(EntityHandle));

	InitializeMassEntityImpl(EntityManager, Fragments);
}

void FFaerieItemInstance::ExportFragmentData(const FMassEntityManager& EntityManager,
	TArray<FInstancedStruct>& OutStructs, const ItemData::EMassFragmentExportOptions Options) const
{
	if (EntityManager.IsEntityValid(EntityHandle))
	{
		const FMassArchetypeHandle Archetype = EntityManager.GetArchetypeForEntity(EntityHandle);
		EntityManager.ForEachArchetypeFragmentType(Archetype,
			[this, &EntityManager, &OutStructs, Options](const UScriptStruct* FragmentType)
			{
				if (EnumHasAnyFlags(Options, ItemData::OnlyFaerieMassFragments))
				{
					if (!FragmentType->IsChildOf<FFaerieMassFragment>())
					{
						return;
					}
				}

				const FStructView StructView = EntityManager.GetFragmentDataStruct(EntityHandle, FragmentType);
				OutStructs.AddDefaulted_GetRef().InitializeAs(StructView.GetScriptStruct(), StructView.GetMemory());
			});
	}
}

void FFaerieItemInstance::AddFragment(FMassEntityManager& EntityManager, FInstancedStruct&& Fragment)
{
	// Adding a fragment is only allowed for mutable instances.
	check(IsMutable())

	if (EntityManager.IsEntityValid(EntityHandle))
	{
		EntityManager.AddFragmentInstanceListToEntity(EntityHandle, MakeArrayView(&Fragment, 1));
		UpdateTimestamp(true);
	}
	else
	{
		InitializeMassEntityImpl(EntityManager, MakeArrayView(&Fragment, 1));
	}

	NotifyOwnerOfChange(EntityManager, Fragment.GetScriptStruct(), ItemData::Tags::FragmentAdd);
}

void FFaerieItemInstance::AddFragments(FMassEntityManager& EntityManager, const TArrayView<FInstancedStruct> Fragments)
{
	// Adding mass fragments are only allowed for mutable instances.
	check(IsMutable())

	if (EntityManager.IsEntityValid(EntityHandle))
	{
		EntityManager.AddFragmentInstanceListToEntity(EntityHandle, Fragments);
		UpdateTimestamp(true);
	}
	else
	{
		InitializeMassEntityImpl(EntityManager, Fragments);
	}

	// @Todo make bundled event API
	for (auto&& Fragment : Fragments)
	{
		NotifyOwnerOfChange(EntityManager, Fragment.GetScriptStruct(), ItemData::Tags::FragmentAdd);
	}
}

void FFaerieItemInstance::RemoveFragment(FMassEntityManager& EntityManager,
										  const TNotNull<const UScriptStruct*> FragmentType)
{
	if (EntityManager.IsEntityValid(EntityHandle))
	{
		EntityManager.RemoveFragmentFromEntity(EntityHandle, FragmentType);

		UpdateTimestamp(true);

		NotifyOwnerOfChange(EntityManager, FragmentType, ItemData::Tags::FragmentRemove);
	}
}

void FFaerieItemInstance::OnItemFragmentEdited(const FMassEntityManager& EntityManager, const TNotNull<const UScriptStruct*> FragmentType, FGameplayTag Tag) const
{
	// @todo figure out how to handle the Tag from the fragment event
	UpdateTimestamp(true);
	NotifyOwnerOfChange(EntityManager, FragmentType, ItemData::Tags::FragmentGenericPropertyEdit);

	// @Todo Subsystem broadcasts...
}

void FFaerieItemInstance::OnItemFragmentEdited(const FMassEntityManager& EntityManager, const TConstStructView<FFaerieMassFragment> FragmentView, const ItemData::FFieldChange& FieldChange) const
{
	UpdateTimestamp(true);
	NotifyOwnerOfChange(EntityManager, FragmentView.GetScriptStruct(), ItemData::Tags::FragmentGenericPropertyEdit);

	const UWorld* World = EntityManager.GetWorld();
	World->GetSubsystemChecked<UFaerieViewModelSubsystem>()->HandleFieldChange(EntityManager, *this, FieldChange);
	World->GetSubsystemChecked<UFaerieMassReplicationSubsystem>()->Server_UpdateFragment(*this, MakeConstArrayView(&FragmentView, 1));
}

FDateTime FFaerieItemInstance::GetLastModified() const
{
	if (FMassEntityManager* EntityManagerPtr = ItemData::GetFaerieEntityManager())
	{
		if (EntityManagerPtr->IsEntityValid(EntityHandle))
		{
			if (auto Ptr = EntityManagerPtr->GetFragmentDataPtr<FFaerieItemModificationDate>(EntityHandle))
			{
				return Ptr->LastModified;
			}
		}
	}

	return FDateTime();
}
