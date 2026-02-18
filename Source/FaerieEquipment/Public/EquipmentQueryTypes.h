// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieSlotTag.h"

#include "EquipmentQueryTypes.generated.h"

struct FFaerieItemStackView;

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBlueprintEquipmentFilter, const FFaerieItemStackView&, View);

USTRUCT(BlueprintType)
struct FFaerieEquipmentQueryTagSet
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "QueryTagSet")
	TSet<FFaerieSlotTag> Tags;
};

USTRUCT(BlueprintType)
struct FFaerieEquipmentBlueprintQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Faerie|EquipmentQuery")
	FBlueprintEquipmentFilter Filter;

	UPROPERTY(BlueprintReadWrite, Category = "Faerie|EquipmentQuery")
	bool InvertFilter = false;
};

USTRUCT(BlueprintType)
struct FFaerieEquipmentSetQuery
{
	GENERATED_BODY()

	// Tags to run the query on.
	UPROPERTY(BlueprintReadWrite, Category = "Faerie|EquipmentSetQuery")
	FFaerieEquipmentQueryTagSet TagSet;

	// Query to run on each tag.
	UPROPERTY(BlueprintReadWrite, Category = "Faerie|EquipmentSetQuery")
	FFaerieEquipmentBlueprintQuery Query;
};