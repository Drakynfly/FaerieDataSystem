// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Capacity/FaerieCapacityHelper.h"
#include "Capacity/InventoryCapacityExtension.h"

#include "FaerieItem.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "MassEntityManager.h"

#include "MassReplication/FaerieViewModelSubsystem.h"

using namespace Faerie;

namespace Faerie::ItemData
{
	namespace
	{
		const FName FieldNames[3]
		{
			GET_MEMBER_NAME_CHECKED(FFaerieItemCapacity, Weight),
			GET_MEMBER_NAME_CHECKED(FFaerieItemCapacity, Bounds),
			GET_MEMBER_NAME_CHECKED(FFaerieItemCapacity, Efficiency)
		};

		const FFieldChange& GetWeightFieldData()
		{
			static const FFieldChange WeightFieldData(FFaerieItemCapacity::StaticStruct(), MakeConstArrayView(FieldNames, 1));
			return WeightFieldData;
		}

		const FFieldChange& GetBoundsFieldData()
		{
			static const FFieldChange BoundsFieldData(FFaerieItemCapacity::StaticStruct(), MakeConstArrayView(FieldNames+1, 1));
			return BoundsFieldData;
		}

		const FFieldChange& GetEfficiencyFieldData()
		{
			static const FFieldChange EfficiencyFieldData(FFaerieItemCapacity::StaticStruct(), MakeConstArrayView(FieldNames+2, 1));
			return EfficiencyFieldData;
		}

		const FFieldChange& GetAllCapacityFieldData()
		{
			static const FFieldChange AllFieldData(FFaerieItemCapacity::StaticStruct(), MakeConstArrayView(FieldNames, 3));
			return AllFieldData;
		}
	}

	FCapacityHelper::FCapacityHelper(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemInstance&> Instance)
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

	void FCapacityHelper::CreateCapacity(FMassEntityManager& InEntityManager, FFaerieItemInstance& Instance, const FFaerieItemCapacity* OverrideDefault)
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
		Instance.AddFragment(InEntityManager, MoveTemp(Fragment));
		if (const FFaerieItemCapacity* CapacityStruct = ItemData::GetEntityFragment<FFaerieItemCapacity>(InEntityManager, Item.GetMassEntityHandle()))
		{
			MassCapacity = CapacityStruct;
		}
	}

	void FCapacityHelper::CreateCapacityIfMissing(FMassEntityManager& InEntityManager, FFaerieItemInstance& Instance, const FFaerieItemCapacity* OverrideDefault)
	{
		if (!MassCapacity)
		{
			CreateCapacity(InEntityManager, Instance, OverrideDefault);
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
				[Instance = Item, NewValue](FMassEntityManager& InEntityManager)
				{
					if (!Instance.IsValid()) return;

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Instance.GetMassEntityHandle());

					// Assign new value
					Fragment.Weight = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
					Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetWeightFieldData());
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
				[Instance = Item, NewValue](FMassEntityManager& InEntityManager)
				{
					if (!Instance.IsValid()) return;

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Instance.GetMassEntityHandle());

					// Assign new value
					Fragment.Bounds = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
					Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetBoundsFieldData());
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
				[Instance = Item, NewValue](FMassEntityManager& InEntityManager)
				{
					if (!Instance.IsValid()) return;

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Instance.GetMassEntityHandle());

					// Assign new value
					Fragment.Efficiency = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
					Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetEfficiencyFieldData());
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
				[Instance = Item, NewValue](FMassEntityManager& InEntityManager)
				{
					if (!Instance.IsValid()) return;

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Instance.GetMassEntityHandle());

					// Assign new value
					Fragment = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
					Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetAllCapacityFieldData());
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
				[Instance = Item, NewValue = GetDefaultCapacity()](FMassEntityManager& InEntityManager)
				{
					if (!Instance.IsValid()) return;

					// Get fragment
					auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Instance.GetMassEntityHandle());

					// Assign new value
					Fragment = NewValue;

					// Broadcast change and tell replication to pass this along to clients.
					const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
					Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetAllCapacityFieldData());
				});

			return;
		}

		checkfSlow(false, TEXT("ResetCapacity should not be called for an item that does not have capacity (and default capacity)"))
	}
}
