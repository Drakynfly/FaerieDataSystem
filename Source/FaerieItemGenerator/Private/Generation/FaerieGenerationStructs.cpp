// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Generation/FaerieGenerationStructs.h"
#include "FaerieItemGenerationLog.h"
#include "ItemInstancingContext_Crafting.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieGenerationStructs)

using namespace Faerie;

TOptional<FFaerieItemStack> FFaerieTableDrop::Resolve(const FFaerieItemInstancingContext_Crafting& Context) const
{
	auto&& DropObject = Asset.Object.LoadSynchronous();

	if (!DropObject || !ensure(DropObject->Implements<UFaerieItemSource>()))
	{
		return NullOpt;
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

		FFaerieItemInstancingContext_Crafting ChildContext;
		ChildContext.Squirrel = Context.Squirrel;

		if (auto StaticInstanceItem = ChildDrop.Resolve(ChildContext))
		{
			if (StaticInstanceItem.IsSet())
			{
				TempContext.GeneratedChildren.Add(StaticResourceSlot.Key, StaticInstanceItem.GetValue());
			}
		}
	}

	return ItemSource->CreateItemStack(&TempContext);
}

void FFaerieGenerationProcedure_OfOne::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
											   TArray<Generation::FPendingTableDrop>& Pending, const int32 Amount) const
{
	const double RanWeight = [Squirrel]
	{
		if (IsValid(Squirrel))
		{
			return Squirrel->NextReal();
		}
		return static_cast<double>(FMath::FRand());
	}();

	if (const FFaerieTableDrop* Drop = Pool.GetDrop(RanWeight))
	{
		Generation::FPendingTableDrop& Result = Pending.AddDefaulted_GetRef();
		Result.Drop = Drop;
		Result.Count = Amount;
		UE_LOG(LogItemGeneration, Log, TEXT("Chose Drop: %s - Amount: %i"), *Result.Drop->Asset.Object.ToString(), Result.Count);
	}
}

void FFaerieGenerationProcedure_OfAny::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
											   TArray<Generation::FPendingTableDrop>& Pending, const int32 Amount) const
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
			Generation::FPendingTableDrop& Result = Pending.AddDefaulted_GetRef();
			Result.Drop = Drop;
			Result.Count = 1;
		}
	}
}

void FFaerieGenerationProcedure_Chunked::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
												 TArray<Generation::FPendingTableDrop>& Pending, int32 Amount) const
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
			Generation::FPendingTableDrop& Result = Pending.AddDefaulted_GetRef();
			Result.Drop = Drop;
			Result.Count = ThisDropAmount;
		}
	}
}

void FFaerieGenerationProcedure_OfAll::Resolve(const FFaerieWeightedPool& Pool, USquirrel* Squirrel,
											   TArray<Generation::FPendingTableDrop>& Pending, const int32 Amount) const
{
	for (const FFaerieWeightedDrop& Drop : Pool.DropList)
	{
		Generation::FPendingTableDrop& Result = Pending.AddDefaulted_GetRef();
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