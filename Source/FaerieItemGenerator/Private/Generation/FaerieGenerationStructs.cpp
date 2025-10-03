// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Generation/FaerieGenerationStructs.h"
#include "FaerieItem.h"
#include "FaerieItemGenerationLog.h"
#include "ItemInstancingContext_Crafting.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieGenerationStructs)

const UFaerieItem* FFaerieTableDrop::Resolve(const FFaerieItemInstancingContext_Crafting& Context) const
{
	auto&& DropObject = Asset.Object.LoadSynchronous();

	if (!DropObject || !ensure(DropObject->Implements<UFaerieItemSource>()))
	{
		return nullptr;
	}

	const IFaerieItemSource* ItemSource = Cast<IFaerieItemSource>(DropObject);
	check(ItemSource);

	FFaerieItemInstancingContext_Crafting TempContext = Context;

	for (auto&& StaticResourceSlot : StaticResourceSlots)
	{
		const FFaerieTableDrop& ChildDrop = StaticResourceSlot.Value.Get<FFaerieTableDrop>();

		// @todo
		// For Subgraph instances, automatically set the stack to the required amount for the filter.
		//UFaerieItemTemplate* Slot;
		//if (UFaerieItemSlotLibrary::FindSlot(DropObject, StaticResourceSlot.Key, Slot))
		//{
		//}

		// Instead of creating millions of these, reuse them somehow. probably have Context contain a temp
		FFaerieItemInstancingContext_Crafting ChildContext;
		ChildContext.Squirrel = Context.Squirrel;

		if (const UFaerieItem* StaticInstanceItem = ChildDrop.Resolve(ChildContext))
		{
			TempContext.GeneratedChildren.Add(StaticResourceSlot.Key, FFaerieItemStack(StaticInstanceItem, 1));
		}
	}

	const UFaerieItem* Item = ItemSource->CreateItemInstance(&TempContext);
	return Item;
}

void FFaerieGenerationProcedure_OfOne::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
											   TArray<Faerie::FPendingItemGeneration>& Pending, const int32 Amount) const
{
	double RanWeight = 0.f;
	if (IsValid(Squirrel))
	{
		RanWeight = Squirrel->NextReal();
	}
	else
	{
		RanWeight = FMath::FRand();
	}

	if (const FFaerieTableDrop* Drop = Pool.GetDrop(RanWeight))
	{
		Faerie::FPendingItemGeneration& Result = Pending.AddDefaulted_GetRef();
		Result.Drop = Drop;
		Result.Count = Amount;
		UE_LOG(LogItemGeneration, Log, TEXT("Chose Drop: %s - Amount: %i"), *Result.Drop->Asset.Object.ToString(), Result.Count);
	}
}

void FFaerieGenerationProcedure_OfAny::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
											   TArray<Faerie::FPendingItemGeneration>& Pending, const int32 Amount) const
{
	for (int32 i = 0; i < Amount; ++i)
	{
		double RanWeight = 0.f;
		if (IsValid(Squirrel))
		{
			RanWeight = Squirrel->NextReal();
		}
		else
		{
			RanWeight = FMath::FRand();
		}

		if (const FFaerieTableDrop* Drop = Pool.GetDrop(RanWeight))
		{
			Faerie::FPendingItemGeneration& Result = Pending.AddDefaulted_GetRef();
			Result.Drop = Drop;
			Result.Count = 1;
		}
	}
}

void FFaerieGenerationProcedure_Chunked::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
												 TArray<Faerie::FPendingItemGeneration>& Pending, int32 Amount) const
{
	while (Amount > 0)
	{
		int32 ThisDropAmount = 0;
		if (IsValid(Squirrel))
		{
			ThisDropAmount = Squirrel->NextInt32InRange(1, Amount);
		}
		else
		{
			ThisDropAmount = FMath::RandRange(1, Amount);
		}

		Amount -= ThisDropAmount;

		double RanWeight = 0.f;
		if (IsValid(Squirrel))
		{
			RanWeight = Squirrel->NextReal();
		}
		else
		{
			RanWeight = FMath::FRand();
		}

		if (const FFaerieTableDrop* Drop = Pool.GetDrop(RanWeight))
		{
			Faerie::FPendingItemGeneration& Result = Pending.AddDefaulted_GetRef();
			Result.Drop = Drop;
			Result.Count = ThisDropAmount;
		}
	}
}

void FFaerieGenerationProcedure_OfAll::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
											   TArray<Faerie::FPendingItemGeneration>& Pending, const int32 Amount) const
{
	for (const FFaerieWeightedDrop& Drop : Pool.DropList)
	{
		Faerie::FPendingItemGeneration& Result = Pending.AddDefaulted_GetRef();
		Result.Drop = &Drop.Drop;
		Result.Count = Amount;
	}
}

int32 FFaerieGeneratorAmount_Range::Resolve(USquirrel* Squirrel) const
{
	return Squirrel->NextInt32InRange(AmountMin, AmountMax);
}

int32 FFaerieGeneratorAmount_Curve::Resolve(USquirrel* Squirrel) const
{
	float Min;
	float Max;
	AmountCurve.GetRichCurveConst()->GetTimeRange(Min, Max);

	const float RawCurveFloat = AmountCurve.GetRichCurveConst()->Eval(static_cast<float>(Squirrel->NextRealInRange(Min, Max)));

	// The following math rounds the RawCurveFloat either up or down based on the remainder. A low remainder is a high
	// chance to round down, while a high remainder is likely to round up.
	int32 const Whole = static_cast<int32>(FMath::Floor(RawCurveFloat));
	float const Remainder = RawCurveFloat - Whole;
	return Whole + (Remainder >= Squirrel->NextReal());
}