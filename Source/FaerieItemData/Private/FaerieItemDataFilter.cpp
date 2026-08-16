// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataFilter.h"
#include "FaerieItemDataView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataFilter)

using namespace Faerie;

#if WITH_EDITOR
bool FFaerieItemDataFilterBase::ExecWithLog(const FMassEntityManager* EntityManager,
	const TValid<const FFaerieItemProxy&> Proxy, ItemData::FFilterLogger& Logger) const
{
	return Exec(EntityManager, Proxy);
}
#endif

bool FFaerieItemDataFilter_Blueprint::Exec(const FMassEntityManager*, const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (Blueprint)
	{
		return Blueprint.GetDefaultObject()->BP_Execute(Proxy);
	}
	return false;
}

#if WITH_EDITOR
bool FFaerieItemDataFilter_Blueprint::ExecWithLog(const FMassEntityManager* EntityManager,
	const TValid<const FFaerieItemProxy&> Proxy, ItemData::FFilterLogger& Logger) const
{
	if (Blueprint)
	{
		return Blueprint.GetDefaultObject()->BP_Execute(Proxy);
	}
	static const FText InvalidBlueprintError = NSLOCTEXT("FFaerieItemDataFilter_Blueprint", "InvalidBlueprint", "Blueprint class is invalid!");
	Logger.Errors.Add(InvalidBlueprintError);
	return false;
}

EFaerieItemDataMutabilityStatus FFaerieItemDataFilter_Blueprint::GetMutabilityStatus() const
{
	if (Blueprint)
	{
		return Blueprint.GetDefaultObject()->GetMutabilityStatus();
	}
	return EFaerieItemDataMutabilityStatus::Unknown;
}
#endif