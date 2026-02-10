// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemDataViewBase.h"
#include "FaerieItemFilterEnums.h"
#include "FaerieItemStackView.h"
#include "FaerieItemDataFilter.generated.h"

struct FFaerieItemDataViewWrapper;

namespace Faerie::ItemData
{
	class IViewBase;

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
	virtual EItemDataMutabilityStatus GetMutabilityStatus() const { return EItemDataMutabilityStatus::Unknown; }
#endif

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataFilter")
	virtual bool Exec(FFaerieItemStackView View) const PURE_VIRTUAL(UFaerieItemDataFilter::Exec, return false; )

	// Overload with ability to log errors. Used by editor validation to collect info about failures.
	virtual bool ExecWithLog(const FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const;

	// Overlord that accepts a ViewBase pointer, typically from an iterator. If children implement this, they also need
	// to implement the struct version as well.
	virtual bool ExecView(Faerie::ItemData::FViewPtr View) const;
};