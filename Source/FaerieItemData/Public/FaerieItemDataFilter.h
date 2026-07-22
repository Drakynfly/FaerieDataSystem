// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemFilterEnums.h"
#include "FaerieItemDataFilter.generated.h"

struct FFaerieItemDataView;

namespace Faerie::ItemData
{
	struct FValidatedDataView;

	class FFilterLogger
	{
	public:
		TArray<FText> Errors;
	};
}

/**
 *
 */
UCLASS(Abstract, Const, EditInlineNew, DefaultToInstanced, CollapseCategories)
class FAERIEITEMDATA_API UFaerieItemDataFilter : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	// This function allows the owning object to know if this filter will allow mutable or immutable assets through, or
	// if it doesn't know. The default is unknown, and specific children must override one way or the other.
	// This function is only called in the editor and saved to a variable when needed at runtime.
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const { return EFaerieItemDataMutabilityStatus::Unknown; }
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const
		PURE_VIRTUAL(UFaerieItemDataFilter::Exec, return false; )

	// Overload with ability to log errors. Used by editor validation to collect info about failures.
	virtual bool ExecWithLog(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View, Faerie::ItemData::FFilterLogger& Logger) const;

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataFilter", meta = (WorldContext = WorldContextObj))
	bool K2_Exec(UObject* WorldContextObj, const FFaerieItemDataView& View) const;
};

UCLASS(Abstract, Blueprintable)
class UFaerieItemDataFilter_BlueprintBase final : public UFaerieItemDataFilter
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override { return EFaerieItemDataMutabilityStatus::Unknown; }
#endif

	virtual bool Exec(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const override;
	virtual bool ExecWithLog(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View, Faerie::ItemData::FFilterLogger& Logger) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Execute"))
	bool BP_Execute(UObject* WorldContextObj, const FFaerieItemDataView& View) const;
};
