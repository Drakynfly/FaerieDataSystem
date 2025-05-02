// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemKey.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemContainerStructs.generated.h"

class UFaerieItemContainerBase;

// Typesafe wrapper around an FFaerieItemKeyBase used for keying entries in a UFaerieItemContainerBase.
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FEntryKey : public FFaerieItemKeyBase
{
	GENERATED_BODY()
	using FFaerieItemKeyBase::FFaerieItemKeyBase;

	static FEntryKey InvalidKey;
};

/**
 * An item container and a key to an entry inside.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FContainerEntryHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InventoryKeyHandle")
	TWeakObjectPtr<UFaerieItemContainerBase> Container;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "InventoryKeyHandle")
	FEntryKey Key;
};

/**
 * Struct to hold the data to save/load an inventory state from.
 */
USTRUCT()
struct FAERIEINVENTORY_API FFaerieContainerSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FInstancedStruct ItemData;

	UPROPERTY(SaveGame)
	TMap<FGuid, FInstancedStruct> ExtensionData;
};