// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"

#include "Fragments/FaerieAssetInfo.h"

#include "StructUtils/StructView.h"

#include "UObject/Object.h"
#include "FaerieItemTemplate.generated.h"

struct FFaerieItemDataFilterBase;

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

	bool TryMatch(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const;
#if WITH_EDITOR
	bool TryMatchWithDescriptions(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, TArray<FText>& Errors) const;
#endif

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemTemplate")
	bool TryMatch(const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemTemplate")
	const FFaerieAssetInfo& GetDescription() const { return Info; }

	TConstStructView<FFaerieItemDataFilterBase> GetFilter() const { return Filter; }

protected:
	UPROPERTY(EditInstanceOnly, Category = "Template")
	FFaerieAssetInfo Info;

	// Filter used to determine if an item qualifies as fitting this template.
	UPROPERTY(EditInstanceOnly, Category = "Template")
	TInstancedStruct<FFaerieItemDataFilterBase> Filter;
};