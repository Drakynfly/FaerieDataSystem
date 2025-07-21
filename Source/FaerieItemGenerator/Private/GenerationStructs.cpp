// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GenerationStructs.h"
#include "FaerieItem.h"
#include "FaerieItemDataStackLiteral.h"
#include "ItemInstancingContext_Crafting.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GenerationStructs)

UFaerieItem* FTableDrop::Resolve(const UItemInstancingContext_Crafting* Context) const
{
	auto&& DropObject = Asset.Object.LoadSynchronous();

	if (!DropObject || !ensure(DropObject->Implements<UFaerieItemSource>()))
	{
		return nullptr;
	}

	const IFaerieItemSource* ItemSource = Cast<IFaerieItemSource>(DropObject);
	check(ItemSource);

	UItemInstancingContext_Crafting* TempContext = DuplicateObject(Context, GetTransientPackage());

	for (auto&& StaticResourceSlot : StaticResourceSlots)
	{
		const FTableDrop& ChildDrop = StaticResourceSlot.Value.Drop.Get<FTableDrop>();

		// @todo
		// For Subgraph instances, automatically set the stack to the required amount for the filter.
		//UFaerieItemTemplate* Slot;
		//if (UFaerieItemSlotLibrary::FindSlot(DropObject, StaticResourceSlot.Key, Slot))
		//{
		//}

		// Instead of creating millions of these, reuse them somehow. probably have Context contain a temp
		UItemInstancingContext_Crafting* ChildContext = NewObject<UItemInstancingContext_Crafting>();
		ChildContext->Squirrel = Context->Squirrel;

		if (UFaerieItem* StaticInstanceItem = ChildDrop.Resolve(ChildContext))
		{
			UFaerieItemDataStackLiteral* Literal = NewObject<UFaerieItemDataStackLiteral>();
			Literal->SetValue(StaticInstanceItem);
			TempContext->InputEntryData.Add(StaticResourceSlot.Key, Literal);
		}
	}

	auto&& Item = ItemSource->CreateItemInstance(TempContext);
	return Item;
}

int32 FGeneratorAmount_Range::Resolve(USquirrel* Squirrel) const
{
	return Squirrel->NextInt32InRange(AmountMin, AmountMax);
}

int32 FGeneratorAmount_Curve::Resolve(USquirrel* Squirrel) const
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