// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemTemplate.h"
#include "FaerieItemDataFilter.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "EntityManagerHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemTemplate)

#define LOCTEXT_NAMESPACE "FaerieItemTemplate"

#if WITH_EDITOR
EDataValidationResult UFaerieItemTemplate::IsDataValid(FDataValidationContext& Context) const
{
	if (!Filter.IsValid())
	{
		Context.AddError(NSLOCTEXT("ValidateFaerieItemTemplate", "InvalidFilterError", "Template Filter is invalid!"));
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(Context);
}
#endif

bool UFaerieItemTemplate::TryMatch(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemProxy&> Proxy) const
{
	if (ensure(Filter.IsValid()))
	{
		return Filter->Exec(EntityManager, Proxy);
	}
	return false;
}

#if WITH_EDITOR
bool UFaerieItemTemplate::TryMatchWithDescriptions(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemProxy&> Proxy, TArray<FText>& Errors) const
{
	if (!ensure(Filter.IsValid()))
	{
		static const FTextFormat GenericErrorFormat = LOCTEXT("ExecWithErrors_InvalidFilter", "'{0}' contains Invalid Filter!'");

		FFormatOrderedArguments Args;
		Args.Add(FText::FromString(GetName()));

		Errors.Add(FText::Format(GenericErrorFormat, Args));
		return false;
	}

	if (Faerie::ItemData::FFilterLogger Logger;
		!Filter->ExecWithLog(EntityManager, Proxy, Logger))
	{
		if (Logger.Errors.IsEmpty())
		{
			static const FTextFormat ErrorFormat = NSLOCTEXT("FaerieItemDataFilter", "GenericFilterError", "Filter '{0}' failed. Implement ExecWithLog for more details.");
			FFormatOrderedArguments Args;
			Args.Add(Filter.GetScriptStruct()->GetDisplayNameText());
			Errors.Add(FText::Format(ErrorFormat, Args));
		}
		else
		{
			Errors.Append(Logger.Errors);
		}

		return false;
	}

	return true;
}
#endif

bool UFaerieItemTemplate::TryMatch(const FFaerieItemProxy& Proxy) const
{
	if (ensure(Filter.IsValid()))
	{
		return Filter->Exec(Faerie::ItemData::GetFaerieEntityManager(), Proxy);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
