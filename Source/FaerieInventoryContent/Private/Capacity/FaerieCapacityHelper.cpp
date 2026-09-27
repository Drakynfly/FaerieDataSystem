// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Capacity/FaerieCapacityHelper.h"

#include "FaerieItem.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "MassEntityManager.h"

using namespace Faerie;

namespace Faerie::ItemData
{
	namespace
	{
		FFieldChangePayload GetWeightFieldData()
		{
			return FFieldChangePayload().SetFlag(FFaerieItemCapacity::EFieldFlags::Weight);
		}

		FFieldChangePayload GetBoundsFieldData()
		{
			return FFieldChangePayload().SetFlag(FFaerieItemCapacity::EFieldFlags::Bounds);
		}

		FFieldChangePayload GetEfficiencyFieldData()
		{
			return FFieldChangePayload().SetFlag(FFaerieItemCapacity::EFieldFlags::Efficiency);
		}

		FFieldChangePayload GetAllCapacityFieldData()
		{
			return FFieldChangePayload().SetFlag(FFaerieItemCapacity::EFieldFlags::All);
		}
	}

	FCapacityHelper::FCapacityHelper(const FMassEntityManager* EntityManager, const FMassEntityHandle Item)
	  : EntityManager(EntityManager), Item(nullptr, Item)
	{
		if (EntityManager)
		{
			if (const FFaerieItemCapacity* CapacityFragment = ItemData::GetEntityFragment<FFaerieItemCapacity>(*EntityManager, Item))
			{
				MassCapacity = CapacityFragment;
			}
		}
	}

	FCapacityHelper::FCapacityHelper(const FMassEntityManager* EntityManager, const FFaerieItemInstance& Instance)
	  : EntityManager(EntityManager), Item(Instance)
	{
		// Look for a live fragment if we have an entity manager
		if (EntityManager)
		{
			if (const FFaerieItemCapacity* CapacityFragment = ItemData::GetEntityFragment<FFaerieItemCapacity>(*EntityManager, Item.GetMassEntityHandle()))
			{
				MassCapacity = CapacityFragment;
			}
		}

		// For the default value.
		if (Item.HasItemAsset())
		{
			if (const FFaerieItemCapacity* DefaultCapacityFragment = Item.GetItemPtr()->GetDefaultFragment<FFaerieItemCapacity>())
            {
            	MassCapacityDefault = DefaultCapacityFragment;
            }
		}
	}

	void FCapacityHelper::CreateCapacity(FMassEntityManager& InEntityManager, const FFaerieItemCapacity* OverrideDefault)
	{
		checkfSlow(!MassCapacity, TEXT("CreateCapacity should not be called for an item that already has capacity"))
		checkfSlow(EntityManager, TEXT("An entity manager is required to initialize mass fragments"))

		FFaerieItemCapacity Capacity;
		if (OverrideDefault)
		{
			Capacity = *OverrideDefault;
		}
		else if (MassCapacityDefault)
		{
			Capacity = *MassCapacityDefault;
		}

		FInstancedStruct Fragment;
		Fragment.InitializeAs<FFaerieItemCapacity>(Capacity);
		FFaerieItemInstance::AddFragment(InEntityManager, Item.GetMassEntityHandle(), MoveTemp(Fragment));
		if (const FFaerieItemCapacity* CapacityStruct = ItemData::GetEntityFragment<FFaerieItemCapacity>(InEntityManager, Item.GetMassEntityHandle()))
		{
			MassCapacity = CapacityStruct;
		}
	}

	void FCapacityHelper::CreateCapacityIfMissing(FMassEntityManager& InEntityManager, const FFaerieItemCapacity* OverrideDefault)
	{
		if (!MassCapacity)
		{
			CreateCapacity(InEntityManager, OverrideDefault);
		}
	}

	bool FCapacityHelper::HasCapacity() const
	{
		return !!MassCapacity || !!MassCapacityDefault;
	}

	FFaerieItemCapacity FCapacityHelper::GetCapacity() const
	{
		if (MassCapacity)
		{
			return *MassCapacity;
		}
		if (MassCapacityDefault)
		{
			return *MassCapacityDefault;
		}

		return FFaerieItemCapacity();
	}

	bool FCapacityHelper::HasDefaultCapacity() const
	{
		return !!MassCapacityDefault;
	}

	const FFaerieItemCapacity& FCapacityHelper::GetDefaultCapacity() const
	{
		return *MassCapacityDefault;
	}

	int32 FCapacityHelper::GetWeightOfStack(const int32 Stack) const
	{
		if (MassCapacity)
		{
			return MassCapacity->Weight * Stack;
		}
		if (MassCapacityDefault)
		{
			return MassCapacityDefault->Weight * Stack;
		}

		checkf(false, TEXT("GetWeightOfStack should not be called for an item that does not have capacity"));
		return 0;
	}

	int64 FCapacityHelper::GetVolumeOfStack(const int32 Stack) const
	{
		if (MassCapacity)
		{
			const int64 Volume = MassCapacity->GetVolume();
			return Volume + static_cast<int64>(Volume * (Stack - 1) * MassCapacity->Efficiency);
		}
		if (MassCapacityDefault)
		{
			const int64 Volume = MassCapacityDefault->GetVolume();
			return Volume + static_cast<int64>(Volume * (Stack - 1) * MassCapacityDefault->Efficiency);
		}
		checkf(false, TEXT("GetVolumeOfStack should not be called for an item that does not have capacity"));
		return 0;
	}

	int64 FCapacityHelper::GetEfficientVolume(const int32 Stack) const
	{
		if (MassCapacity)
		{
			const int64 Volume = MassCapacity->GetVolume();
			return static_cast<int64>(Volume * Stack * MassCapacity->Efficiency);
		}
		if (MassCapacityDefault)
		{
			const int64 Volume = MassCapacityDefault->GetVolume();
			return static_cast<int64>(Volume * Stack * MassCapacityDefault->Efficiency);
		}
		checkf(false, TEXT("GetEfficientVolume should not be called for an item that does not have capacity"));
		return 0;
	}

	FFaerieWeightAndVolume FCapacityHelper::GetWeightAndVolumeOfStack(const int32 Stack) const
	{
		return FFaerieWeightAndVolume(GetWeightOfStack(Stack), GetVolumeOfStack(Stack));
	}

	FFaerieWeightAndVolume FCapacityHelper::GetWeightAndVolumeOfPartialStack(const int32 Stack) const
	{
		return FFaerieWeightAndVolume(GetWeightOfStack(Stack), GetEfficientVolume(Stack));
	}

	void FCapacityHelper::SetWeight(const int32 NewValue)
	{
		if (MassCapacity)
		{
			EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
				[Entity = Item.GetMassEntityHandle(), NewValue](FMassEntityManager& InEntityManager)
				{
					if (!InEntityManager.IsEntityValid(Entity))
					{
						return;
					}

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Entity);

					// Assign new value
					Fragment.Weight = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					FFaerieItemInstance::OnItemFragmentEdited(InEntityManager, Entity, FFaerieItemCapacity::StaticStruct(), GetWeightFieldData());
				});

			return;
		}

		checkfSlow(false, TEXT("SetBounds should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::SetBounds(const FIntVector& NewValue)
	{
		if (MassCapacity)
		{
			EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
				[Entity = Item.GetMassEntityHandle(), NewValue](FMassEntityManager& InEntityManager)
				{
					if (!InEntityManager.IsEntityValid(Entity))
					{
						return;
					}

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Entity);

					// Assign new value
					Fragment.Bounds = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					FFaerieItemInstance::OnItemFragmentEdited(InEntityManager, Entity, FFaerieItemCapacity::StaticStruct(), GetBoundsFieldData());
				});

			return;
		}

		checkfSlow(false, TEXT("SetBounds should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::SetEfficiency(const float NewValue)
	{
		if (MassCapacity)
		{
			EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
				[Entity = Item.GetMassEntityHandle(), NewValue](FMassEntityManager& InEntityManager)
				{
					if (!InEntityManager.IsEntityValid(Entity))
					{
						return;
					}

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Entity);

					// Assign new value
					Fragment.Efficiency = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					FFaerieItemInstance::OnItemFragmentEdited(InEntityManager, Entity, FFaerieItemCapacity::StaticStruct(), GetEfficiencyFieldData());
				});

			return;
		}

		checkfSlow(false, TEXT("SetEfficiency should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::SetCapacity(const FFaerieItemCapacity& NewValue)
	{
		if (MassCapacity)
		{
			EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
				[Entity = Item.GetMassEntityHandle(), NewValue](FMassEntityManager& InEntityManager)
				{
					if (!InEntityManager.IsEntityValid(Entity))
					{
						return;
					}

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Entity);

					// Assign new value
					Fragment = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					FFaerieItemInstance::OnItemFragmentEdited(InEntityManager, Entity, FFaerieItemCapacity::StaticStruct(), GetAllCapacityFieldData());
				});

			return;
		}

		checkfSlow(false, TEXT("SetCapacity should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::ResetCapacity()
	{
		if (MassCapacity && MassCapacityDefault)
		{
			EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
				[Entity = Item.GetMassEntityHandle(), NewValue = GetDefaultCapacity()](FMassEntityManager& InEntityManager)
				{
					if (!InEntityManager.IsEntityValid(Entity))
					{
						return;
					}

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Entity);

					// Assign new value
					Fragment = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					FFaerieItemInstance::OnItemFragmentEdited(InEntityManager, Entity, FFaerieItemCapacity::StaticStruct(), GetAllCapacityFieldData());
				});

			return;
		}

		checkfSlow(false, TEXT("ResetCapacity should not be called for an item that does not have capacity (and default capacity)"))
	}
}
