// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CapacityStructsLibrary.h"
#include "Capacity/FaerieCapacityHelper.h"
#include "Capacity/InventoryCapacityExtension.h"
#include "FaerieItemProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CapacityStructsLibrary)

FFaerieItemCapacity UFaerieCapacityStructsUtilities::GetCapacity(const FFaerieItemProxy& Proxy)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieCapacityStructsUtilities::GetCapacity"), ELogVerbosity::Error);
		return FFaerieItemCapacity();
	}

	const FMassEntityManager* EntityManager = nullptr;
	if (InstanceOpt.GetValue().HasMassEntity())
	{
		EntityManager = Proxy.GetEntityManager().Get();
	}

	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, InstanceOpt.GetValue());
	if (Capacity.HasFragmentValue())
	{
		return *Capacity.GetFragmentValue();
	}

	return FFaerieItemCapacity();
}

int32 UFaerieCapacityStructsUtilities::GetWeightOfStack(const FFaerieItemProxy& Proxy, const int32 Stack)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieCapacityStructsUtilities::GetWeightOfStack"), ELogVerbosity::Error);
		return 0;
	}

	const FMassEntityManager* EntityManager = nullptr;
	if (InstanceOpt.GetValue().HasMassEntity())
	{
		EntityManager = Proxy.GetEntityManager().Get();
	}

	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, InstanceOpt.GetValue());
	if (Capacity.HasFragmentValue())
	{
		return Capacity.GetWeightOfStack(Stack);
	}
	return 0;
}

int64 UFaerieCapacityStructsUtilities::GetVolumeOfStack(const FFaerieItemProxy& Proxy, const int32 Stack)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieCapacityStructsUtilities::GetVolumeOfStack"), ELogVerbosity::Error);
		return 0;
	}

	const FMassEntityManager* EntityManager = nullptr;
	if (InstanceOpt.GetValue().HasMassEntity())
	{
		EntityManager = Proxy.GetEntityManager().Get();
	}

	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, InstanceOpt.GetValue());
	if (Capacity.HasFragmentValue())
	{
		return Capacity.GetVolumeOfStack(Stack);
	}
	return 0;
}

int64 UFaerieCapacityStructsUtilities::GetEfficientVolume(const FFaerieItemProxy& Proxy, const int32 Stack)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieCapacityStructsUtilities::GetEfficientVolume"), ELogVerbosity::Error);
		return 0;
	}

	const FMassEntityManager* EntityManager = nullptr;
	if (InstanceOpt.GetValue().HasMassEntity())
	{
		EntityManager = Proxy.GetEntityManager().Get();
	}

	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, InstanceOpt.GetValue());
	if (Capacity.HasFragmentValue())
	{
		return Capacity.GetEfficientVolume(Stack);
	}
	return 0;
}

FFaerieWeightAndVolume UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfStack(const FFaerieItemProxy& Proxy, const int32 Stack)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfStack"), ELogVerbosity::Error);
		return FFaerieWeightAndVolume();
	}

	const FMassEntityManager* EntityManager = nullptr;
	if (InstanceOpt.GetValue().HasMassEntity())
	{
		EntityManager = Proxy.GetEntityManager().Get();
	}

	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, InstanceOpt.GetValue());
	if (Capacity.HasFragmentValue())
	{
		return Capacity.GetWeightAndVolumeOfStack(Stack);
	}
	return FFaerieWeightAndVolume();
}

FFaerieWeightAndVolume UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfPartialStack(const FFaerieItemProxy& Proxy, const int32 Stack)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfPartialStack"), ELogVerbosity::Error);
		return FFaerieWeightAndVolume();
	}

	const FMassEntityManager* EntityManager = nullptr;
	if (InstanceOpt.GetValue().HasMassEntity())
	{
		EntityManager = Proxy.GetEntityManager().Get();
	}

	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, InstanceOpt.GetValue());
	if (Capacity.HasFragmentValue())
	{
		return Capacity.GetWeightAndVolumeOfPartialStack(Stack);
	}
	return FFaerieWeightAndVolume();
}

FFaerieItemCapacity UFaerieCapacityStructsUtilities::WeightOfScaledComparison(const FFaerieItemCapacity& Original,
																		const FFaerieItemCapacity& Comparison)
{
	FFaerieItemCapacity Out = Original;
	Out.Weight = static_cast<int32>(Original.GetEfficientVolume() * Comparison.WeightOfSquareCentimeter());
	return Out;
}

FFaerieWeightAndVolume UFaerieCapacityStructsUtilities::ToWeightAndVolume_ItemCapacity(const FFaerieItemCapacity& ItemCapacity)
{
	return FFaerieWeightAndVolume(ItemCapacity.Weight, ItemCapacity.GetVolume());
}

FFaerieWeightAndVolume UFaerieCapacityStructsUtilities::GetCurrentCapacity(const FFaerieCapacityExtensionState& State)
{
	return FFaerieWeightAndVolume(State.CurrentWeight, State.CurrentVolume);
}

FFaerieWeightAndVolume UFaerieCapacityStructsUtilities::GetMaxCapacity(const FFaerieCapacityExtensionConfig& Config)
{
	return FFaerieWeightAndVolume(Config.MaxWeight, Config.MaxVolume);
}

bool UFaerieCapacityStructsUtilities::IsOverMaxWeight(const FFaerieCapacityExtensionState& State, const FFaerieCapacityExtensionConfig& Config)
{
	return State.CurrentWeight > Config.MaxWeight;
}

bool UFaerieCapacityStructsUtilities::IsOverMaxVolume(const FFaerieCapacityExtensionState& State, const FFaerieCapacityExtensionConfig& Config)
{
	return State.CurrentVolume > Config.MaxVolume;
}

float UFaerieCapacityStructsUtilities::GetPercentageFullForWeightAndVolume(const FFaerieWeightAndVolume& WeightAndVolume, const FFaerieCapacityExtensionConfig& Config)
{
	float ScalarWeightFull = 0;
	float ScalarVolumeFull = 0;

	if (Config.MaxWeight > 0)
	{
		ScalarWeightFull = static_cast<float>(WeightAndVolume.GramWeight) / static_cast<float>(Config.MaxWeight);
	}

	if (Config.MaxVolume > 0)
	{
		ScalarVolumeFull = static_cast<float>(WeightAndVolume.Volume) / static_cast<float>(Config.MaxVolume);
	}

	const float LargerFull = FMath::Max(ScalarWeightFull, ScalarVolumeFull);
	const float SmallerFull = FMath::Min(ScalarWeightFull, ScalarVolumeFull);

	const float SecondAmountToFill = 1 - LargerFull;
	return LargerFull + (SmallerFull * SecondAmountToFill);
}

float UFaerieCapacityStructsUtilities::GetPercentageFull(const FFaerieCapacityExtensionState& State, const FFaerieCapacityExtensionConfig& Config)
{
	return GetPercentageFullForWeightAndVolume(GetCurrentCapacity(State), Config);
}