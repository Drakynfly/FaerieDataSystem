// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemTemplate.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemStackView.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemTemplate)

#define LOCTEXT_NAMESPACE "FaerieItemTemplate"

#if WITH_EDITOR

EDataValidationResult UFaerieItemTemplate::IsDataValid(FDataValidationContext& Context) const
{
	if (!IsValid(Pattern))
	{
		Context.AddError(NSLOCTEXT("ValidateFaerieItemTemplate", "InvalidPatternError", "Template Pattern is invalid!"));
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(Context);
}

#endif

bool UFaerieItemTemplate::TryMatchWithDescriptions(const FFaerieItemStackView View, TArray<FText>& Errors) const
{
	if (!ensure(IsValid(Pattern)))
	{
		static const FTextFormat GenericErrorFormat = LOCTEXT("ExecWithErrors_InvalidPattern", "'{0}' contains Invalid Pattern!'");

		FFormatOrderedArguments Args;
		Args.Add(FText::FromString(GetName()));

		Errors.Add(FText::Format(GenericErrorFormat, Args));
		return false;
	}

	if (Faerie::ItemData::FFilterLogger Logger;
		!Pattern->ExecWithLog(View, Logger))
	{
		if (Logger.Errors.IsEmpty())
		{
			static const FTextFormat GenericErrorFormat = LOCTEXT("ExecWithErrors_GenericErrorFmt", "'{0}' failed with unspecified reason!'");

			FFormatOrderedArguments Args;
			Args.Add(FText::FromString(GetName()));

			Errors.Add(FText::Format(GenericErrorFormat, Args));
		}
		else
		{
			Errors = Logger.Errors;
		}

		return false;
	}

	return true;
}

bool UFaerieItemTemplate::TryMatch(const FFaerieItemStackView View) const
{
	if (ensure(IsValid(Pattern)))
	{
		return Pattern->Exec(View);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE