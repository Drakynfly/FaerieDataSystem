// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "CapacityStructsLibrary.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"

#include "Capacity/FaerieCapacityHelper.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CapacityStructsLibrary)

FFaerieItemCapacity UFaerieCapacityStructsUtilities::GetCapacity(UObject* WorldContextObj, const FFaerieItemInstance& Instance)
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieCapacityStructsUtilities::GetCapacity"), ELogVerbosity::Error);
		return FFaerieItemCapacity();
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Item passed to UFaerieCapacityStructsUtilities::GetCapacity"), ELogVerbosity::Error);
		return FFaerieItemCapacity();
	}

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, Instance);
	if (Capacity.HasCapacity())
	{
		return Capacity.GetCapacity();
	}
	return FFaerieItemCapacity();
}

int32 UFaerieCapacityStructsUtilities::GetWeightOfStack(UObject* WorldContextObj, const FFaerieItemInstance& Instance, const int32 Stack)
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieCapacityStructsUtilities::GetWeightOfStack"), ELogVerbosity::Error);
		return 0;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Item passed to UFaerieCapacityStructsUtilities::GetWeightOfStack"), ELogVerbosity::Error);
		return 0;
	}

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, Instance);
	if (Capacity.HasCapacity())
	{
		return Capacity.GetWeightOfStack(Stack);
	}
	return 0;
}

int64 UFaerieCapacityStructsUtilities::GetVolumeOfStack(UObject* WorldContextObj, const FFaerieItemInstance& Instance, const int32 Stack)
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieCapacityStructsUtilities::GetVolumeOfStack"), ELogVerbosity::Error);
		return 0;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Item passed to UFaerieCapacityStructsUtilities::GetVolumeOfStack"), ELogVerbosity::Error);
		return 0;
	}

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, Instance);
	if (Capacity.HasCapacity())
	{
		return Capacity.GetVolumeOfStack(Stack);
	}
	return 0;
}

int64 UFaerieCapacityStructsUtilities::GetEfficientVolume(UObject* WorldContextObj, const FFaerieItemInstance& Instance, const int32 Stack)
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieCapacityStructsUtilities::GetEfficientVolume"), ELogVerbosity::Error);
		return 0;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Item passed to UFaerieCapacityStructsUtilities::GetEfficientVolume"), ELogVerbosity::Error);
		return 0;
	}

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, Instance);
	if (Capacity.HasCapacity())
	{
		return Capacity.GetEfficientVolume(Stack);
	}
	return 0;
}

FFaerieWeightAndVolume UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfStack(UObject* WorldContextObj,
	const FFaerieItemInstance& Instance, const int32 Stack)
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfStack"), ELogVerbosity::Error);
		return FFaerieWeightAndVolume();
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Item passed to UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfStack"), ELogVerbosity::Error);
		return FFaerieWeightAndVolume();
	}

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, Instance);
	if (Capacity.HasCapacity())
	{
		return Capacity.GetWeightAndVolumeOfStack(Stack);
	}
	return FFaerieWeightAndVolume();
}

FFaerieWeightAndVolume UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfPartialStack(UObject* WorldContextObj,
	const FFaerieItemInstance& Instance, const int32 Stack)
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfPartialStack"), ELogVerbosity::Error);
		return FFaerieWeightAndVolume();
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieCapacityStructsUtilities::GetWeightAndVolumeOfPartialStack"), ELogVerbosity::Error);
		return FFaerieWeightAndVolume();
	}

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	const Faerie::ItemData::FCapacityHelper Capacity(EntityManager, Instance);
	if (Capacity.HasCapacity())
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