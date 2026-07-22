// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataFilter.h"
#include "FaerieItemDataView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataFilter)

bool UFaerieItemDataFilter::ExecWithLog(const TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View,
										Faerie::ItemData::FFilterLogger& Logger) const
{
	const bool Result = Exec(WorldContextObj, View);
	if (!Result)
	{
		static const FTextFormat ErrorFormat = NSLOCTEXT("FaerieItemDataFilter", "GenericFilterError", "Filter '{0}' failed. Implement ExecWithLog for more details.");
		FFormatOrderedArguments Args;
#if WITH_EDITOR
		Args.Add(GetClass()->GetDisplayNameText());
#else
		Args.Add(FText::FromString(GetClass()->GetName()));
#endif
		Logger.Errors.Add(FText::Format(ErrorFormat, Args));
	}

	return Result;
}

bool UFaerieItemDataFilter::K2_Exec(UObject* WorldContextObj, const FFaerieItemDataView& View) const
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieItemDataFilter::K2_Exec"), ELogVerbosity::Error);
		return false;
	}

	return Exec(TNotNull<const UObject*>(WorldContextObj), Faerie::ItemData::FValidatedDataView(View));
}

bool UFaerieItemDataFilter_BlueprintBase::Exec(const TNotNull<const UObject*> WorldContextObj,
	const Faerie::ItemData::FValidatedDataView& View) const
{
	return BP_Execute(const_cast<UObject*>(NotNullGet(WorldContextObj)), View);
}

bool UFaerieItemDataFilter_BlueprintBase::ExecWithLog(const TNotNull<const UObject*> WorldContextObj,
	const Faerie::ItemData::FValidatedDataView& View, Faerie::ItemData::FFilterLogger& Logger) const
{
	return BP_Execute(const_cast<UObject*>(NotNullGet(WorldContextObj)), View);
}
