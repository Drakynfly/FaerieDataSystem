// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Capacity/FaerieCapacityHelper.h"

#include "FaerieItem.h"
#include "FaerieItemInstance.h"
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

	FCapacityHelper::FCapacityHelper(const FMassEntityManager& EntityManager, const FMassEntityHandle Item)
	  : EntityManager(&EntityManager), EntityHandle(Item)
	{
		FragmentPtr = ItemData::GetEntityFragment<FFaerieItemCapacity>(EntityManager, Item);
		Defaults_FragmentPtr = ItemData::GetDefaultFragment<FFaerieItemCapacity>(EntityManager, Item).GetPtr();
	}

	FCapacityHelper::FCapacityHelper(const FFaerieItemProxy& Item)
	  : EntityManager(Item.GetEntityManager().Get()), EntityHandle(Item.GetEntityHandle())
	{
		FragmentPtr = ItemData::GetEntityFragment<FFaerieItemCapacity>(Item);
		Defaults_FragmentPtr = ItemData::GetDefaultFragment<FFaerieItemCapacity>(Item).GetPtr();
	}

	void FCapacityHelper::CreateFragment(FMassEntityManager& InEntityManager, const FFaerieItemCapacity* OverrideDefault)
	{
		checkfSlow(!FragmentPtr, TEXT("CreateCapacity should not be called for an item that already has capacity"))
		checkfSlow(EntityManager, TEXT("An entity manager is required to initialize mass fragments"))

		FFaerieItemCapacity Capacity;
		if (OverrideDefault)
		{
			Capacity = *OverrideDefault;
		}
		else if (Defaults_FragmentPtr)
		{
			Capacity = *Defaults_FragmentPtr;
		}

		FInstancedStruct Fragment;
		Fragment.InitializeAs<FFaerieItemCapacity>(Capacity);
		ItemData::AddFragment(InEntityManager, EntityHandle, MoveTemp(Fragment));
		if (const FFaerieItemCapacity* CapacityStruct = ItemData::GetEntityFragment<FFaerieItemCapacity>(InEntityManager, EntityHandle))
		{
			FragmentPtr = CapacityStruct;
		}
	}

	void FCapacityHelper::Mutate(TFunctionRef<FFieldChangePayload(FCapacityHelper& Helper, FFaerieItemCapacity&)> Func)
	{
		checkf(HasFragmentValue(), TEXT("Cannot mutate a fragment that has no value."))

		EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
			[Entity = EntityHandle, Func](FMassEntityManager& InEntityManager)
			{
				if (!InEntityManager.IsEntityValid(Entity))
				{
					// Mutation canceled, entity no longer exists.
					return;
				}

				// Assign new value
				FCapacityHelper Helper(InEntityManager, Entity);
				Helper.CreateFragmentIfMissing(InEntityManager);
				FFaerieItemCapacity& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemCapacity>(Entity);
				const FFieldChangePayload Payload = Func(Helper, Fragment);

				// Broadcast change and tell replication to pass this along to clients.
				OnItemFragmentEdited(InEntityManager, Entity, FFaerieItemCapacity::StaticStruct(), Payload);
			});
	}

	int32 FCapacityHelper::GetWeightOfStack(const int32 Stack) const
	{
		if (HasFragmentValue())
		{
			return GetFragmentValue()->Weight * Stack;
		}
		checkf(false, TEXT("GetWeightOfStack should not be called for an item that does not have capacity"));
		return 0;
	}

	int64 FCapacityHelper::GetVolumeOfStack(const int32 Stack) const
	{
		if (HasFragmentValue())
		{
			const int64 Volume = GetFragmentValue()->GetVolume();
			return Volume + static_cast<int64>(static_cast<double>(Volume * (Stack - 1)) * static_cast<double>(GetFragmentValue()->Efficiency));
		}
		checkf(false, TEXT("GetVolumeOfStack should not be called for an item that does not have capacity"));
		return 0;
	}

	int64 FCapacityHelper::GetEfficientVolume(const int32 Stack) const
	{
		if (HasFragmentValue())
		{
			const int64 Volume = GetFragmentValue()->GetVolume();
			return static_cast<int64>(static_cast<double>(Volume * Stack) * static_cast<double>(GetFragmentValue()->Efficiency));
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
		if (HasFragmentValue())
		{
			Mutate([NewValue](FCapacityHelper& Helper, FFaerieItemCapacity& Capacity)
			{
				// Assign new value
				Capacity.Weight = NewValue;
				return GetWeightFieldData();
			});

			return;
		}

		checkfSlow(false, TEXT("SetBounds should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::SetBounds(const FIntVector& NewValue)
	{
		if (HasFragmentValue())
		{
			Mutate([NewValue](FCapacityHelper& Helper, FFaerieItemCapacity& Capacity)
			{
				// Assign new value
				Capacity.Bounds = NewValue;
				return GetBoundsFieldData();
			});

			return;
		}

		checkfSlow(false, TEXT("SetBounds should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::SetEfficiency(const float NewValue)
	{
		if (HasFragmentValue())
		{
			Mutate([NewValue](FCapacityHelper& Helper, FFaerieItemCapacity& Capacity)
			{
				// Assign new value
				Capacity.Efficiency = NewValue;
				return GetEfficiencyFieldData();
			});
			return;
		}

		checkfSlow(false, TEXT("SetEfficiency should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::SetCapacity(const FFaerieItemCapacity& NewValue)
	{
		if (HasFragmentValue())
		{
			Mutate([NewValue](FCapacityHelper& Helper, FFaerieItemCapacity& Capacity)
			{
				// Assign new value
				Capacity = NewValue;
				return GetAllCapacityFieldData();
			});

			return;
		}

		checkfSlow(false, TEXT("SetCapacity should not be called for an item that does not have capacity"))
	}

	void FCapacityHelper::ResetCapacity()
	{
		// Resetting is only valid, if there is a "default" to reset to.
		if (HasDefaultValue())
		{
			Mutate([NewValue = GetDefaultValue()](FCapacityHelper& Helper, FFaerieItemCapacity& Capacity)
			{
				// Assign new value
				Capacity = NewValue;
				return GetAllCapacityFieldData();
			});

			return;
		}

		checkfSlow(false, TEXT("ResetCapacity should not be called for an item that does not have capacity (and default capacity)"))
	}
}
