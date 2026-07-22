// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Fragments/FaerieAssetInfo.h"

#include "UObject/Object.h"
#include "FaerieItemTemplate.generated.h"

namespace Faerie::ItemData
{
	struct FValidatedDataView;
}

struct FFaerieItemDataView;
class UFaerieItemDataFilter;

/**
 * A wrapper around an ItemDataFilter, used by Item Assets to validate what they generate.
 * Also used as a "description" type, to declare a pattern of what a *theoretical* item would look like.
 */
UCLASS(BlueprintType, const)
class FAERIEITEMDATA_API UFaerieItemTemplate : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	bool TryMatch(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const;
	bool TryMatchWithDescriptions(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View, TArray<FText>& Errors) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemTemplate", meta = (WorldContext = "WorldContextObj"))
	bool TryMatch(UObject* WorldContextObj, const FFaerieItemDataView& View) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemTemplate")
	const FFaerieAssetInfo& GetDescription() const { return Info; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemTemplate")
	const UFaerieItemDataFilter* GetPattern() const { return Pattern; }

protected:
	UPROPERTY(EditInstanceOnly, Category = "Template")
	FFaerieAssetInfo Info;

	// Pattern used to determine if an item qualifies as fitting this template.
	UPROPERTY(EditInstanceOnly, Category = "Template", meta = (DisplayThumbnail = false))
	TObjectPtr<UFaerieItemDataFilter> Pattern;
};