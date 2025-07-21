// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "EquipmentHashAsset.generated.h"

class UFaerieItemStackHashInstruction;

USTRUCT()
struct FFaerieEquipmentHashAssetConfig
{
	GENERATED_BODY()

	// The slots to check the instruction on.
	UPROPERTY(EditAnywhere, Category = "EquipmentHashInstruction", meta = (Categories = "Fae.Slot"))
	FGameplayTagContainer Slots;

	// Should all slots be hashed together for this instruction, or only the first non-empty?
	UPROPERTY(EditAnywhere, Category = "EquipmentHashInstruction")
	EGameplayContainerMatchType MatchType = EGameplayContainerMatchType::All;

	UPROPERTY(EditAnywhere, Instanced, Category = "EquipmentHashInstruction")
	TObjectPtr<UFaerieItemStackHashInstruction> Instruction;

#if WITH_EDITORONLY_DATA
	// An example of a itemset that should pass. Used to generate CheckHash
	UPROPERTY(EditAnywhere, Category = "EquipmentHashInstruction")
	TArray<TObjectPtr<class UFaerieItemAsset>> Example;
#endif
};

/**
 *
 */
UCLASS(Const, BlueprintType)
class FAERIEEQUIPMENT_API UFaerieEquipmentHashAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	UPROPERTY(EditInstanceOnly, Category = "EquipmentHashAsset")
	TArray<FFaerieEquipmentHashAssetConfig> Configs;

	UPROPERTY(VisibleAnywhere, Category = "EquipmentHashAsset")
	int32 CheckHash;
};