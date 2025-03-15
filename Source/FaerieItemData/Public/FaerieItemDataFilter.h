// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemDataProxy.h"
#include "FaerieItemFilterTypes.h"
#include "FaerieItemDataFilter.generated.h"

namespace Faerie::ItemData
{
	class FFilterLogger
	{
	public:
		TArray<FText> Errors;
	};
}

// @todo convert these to an struct, and implement with TInstancedStruct
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

	virtual bool ExecWithLog(const FFaerieItemStackView View, Faerie::ItemData::FFilterLogger& Logger) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataFilter")
	virtual bool Exec(FFaerieItemStackView View) const PURE_VIRTUAL(UFaerieItemDataFilter::Exec, return false; )
};

USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FInlineFaerieItemDataFilter
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Inline Inventory Filter Filter")
	TObjectPtr<UFaerieItemDataFilter> Filter = nullptr;
};