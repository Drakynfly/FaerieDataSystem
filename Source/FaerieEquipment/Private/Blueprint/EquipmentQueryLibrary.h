// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieSlotTag.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EquipmentQueryLibrary.generated.h"

struct FFaerieItemInstance;
struct FFaerieEquipmentSetQuery;
class UFaerieEquipmentHashAsset;
class UFaerieEquipmentManager;
class UFaerieEquipmentSlot;

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(int32, FBlueprintEquipmentHash, const UObject*, WorldContextObj, const FFaerieItemInstance&, Instance);

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

/**
 *
 */
UCLASS()
class UFaerieEquipmentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns true if the values are equal (A == B) */
	UFUNCTION(BlueprintPure, Category = "GameplayTags", meta = (DisplayName = "Equal (FaerieSlotTag)", CompactNodeTitle = "==", BlueprintThreadSafe))
	static bool EqualEqual_FaerieSlotTag(const FFaerieSlotTag A, const FFaerieSlotTag B) { return A == B; }

	/** Returns true if the values are not equal (A != B) */
	UFUNCTION(BlueprintPure, Category = "GameplayTags", meta = (DisplayName = "Not Equal (FaerieSlotTag)", CompactNodeTitle = "!=", BlueprintThreadSafe))
	static bool NotEqual_FaerieSlotTag(const FFaerieSlotTag A, const FFaerieSlotTag B)  { return A != B; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentQuery")
	static bool RunEquipmentQuery(UFaerieEquipmentManager* Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot);

	// Generate a hash from a set of slots. Typically used for checksum'ing.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentHashing")
	static FFaerieHash HashEquipment(const UFaerieEquipmentManager* Manager, const FFaerieEquipmentHashConfig& Config);

	// Generate a hash from a set of slots, using a predefined asset.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentHashing")
	static bool ExecuteHashInstructions(const UFaerieEquipmentManager* Manager, const UFaerieEquipmentHashAsset* Asset);

	UFUNCTION(BlueprintPure, Category = "Faerie|EquipmentHashing")
	static FBlueprintEquipmentHash GetEquipmentHash_ByName();

protected:
	static int32 ExecHashItemByName(UObject* WorldContextObj, const FFaerieItemInstance& Instance);
};