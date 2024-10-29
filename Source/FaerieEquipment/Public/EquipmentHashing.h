﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "EquipmentHashTypes.h"
#include "FaerieSlotTag.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EquipmentHashing.generated.h"

struct FFaerieItemStackView;
class UFaerieEquipmentSlot;
class UFaerieEquipmentHashAsset;

class UFaerieItem;
using FEquipmentHashFunction = TFunctionRef<int32(const UFaerieItem*)>;
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(int32, FBlueprintEquipmentHash, const UFaerieItem*, Item);


USTRUCT(BlueprintType)
struct FFaerieEquipmentHashConfig
{
	GENERATED_BODY()

	// Which slots to use in the hash
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentHashConfig")
	TSet<FFaerieSlotTag> Slots;

	// Hash function
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentHashConfig")
	FBlueprintEquipmentHash HashFunction;
};

class UFaerieEquipmentManager;

namespace Faerie::Hash
{
	// Get the hash of a FProperty's value on a specific object
	FAERIEEQUIPMENT_API int32 HashFProperty(const void* Ptr, const FProperty* Property);
	FAERIEEQUIPMENT_API int32 HashFProperty(const UObject* Obj, const FProperty* Property);

	FAERIEEQUIPMENT_API int32 HashStructByProps(const void* Ptr, const UScriptStruct* Struct, bool IncludeSuper);
	FAERIEEQUIPMENT_API int32 HashObjectByProps(const UObject* Obj, bool IncludeSuper);
}

/**
 *
 */
UCLASS()
class FAERIEEQUIPMENT_API UFaerieEquipmentHashing : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FFaerieEquipmentHash MakeEquipmentHash(TConstArrayView<int32> Hashes);

	static FFaerieEquipmentHash HashItemSet(const TSet<const UFaerieItem*>& Items, const FEquipmentHashFunction& Function);

	static FFaerieEquipmentHash HashEquipment(const UFaerieEquipmentManager* Manager, const TSet<FFaerieSlotTag>& Slots, const FEquipmentHashFunction& Function);

	// Generate a hash from a set of slots. Typically used for checksum'ing.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentHashing")
	static FFaerieEquipmentHash HashEquipment(const UFaerieEquipmentManager* Manager, const FFaerieEquipmentHashConfig& Config);

	// Generate a hash from a set of slots, using a predefined asset.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentHashing")
	static bool ExecuteHashInstructions(const UFaerieEquipmentManager* Manager, const UFaerieEquipmentHashAsset* Asset);

	UFUNCTION(BlueprintPure, Category = "Faerie|EquipmentHashing")
	static FBlueprintEquipmentHash GetEquipmentHash_ByName();

	static int32 ExecHashEquipmentByName(const UFaerieItem* Item);
};


using FNativeEquipmentFilter = TDelegate<bool(const FFaerieItemStackView&)>;
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBlueprintEquipmentFilter, const FFaerieItemStackView&, View);

struct FFaerieEquipmentNativeQuery
{
	FNativeEquipmentFilter Filter;
	bool InvertFilter;
};

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

/**
 *
 */
UCLASS()
class FAERIEEQUIPMENT_API UFaerieEquipmentQueryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentQuery")
	static bool RunEquipmentQuery(const UFaerieEquipmentManager* Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot);
};