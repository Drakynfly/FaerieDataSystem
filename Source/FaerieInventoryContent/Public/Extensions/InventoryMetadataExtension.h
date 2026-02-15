// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "GameplayTagContainer.h"
#include "InventoryReplicatedDataExtensionBase.h"
#include "TypedGameplayTags.h"
#include "InventoryMetadataExtension.generated.h"

/**
 * The key used to tag entries with custom client data.
 */
USTRUCT(BlueprintType, meta = (Categories = "Fae.Inventory.Meta"))
struct FAERIEINVENTORYCONTENT_API FFaerieInventoryMetaTag : public FFaerieInventoryTag
{
	GENERATED_BODY()
	END_TAG_DECL2(FFaerieInventoryMetaTag, TEXT("Fae.Inventory.Meta"))
};

// Server-only metadata flags
namespace Faerie::Inventory::Tags
{
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryMetaTag, CannotRemove)
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryMetaTag, CannotDelete)
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryMetaTag, CannotMove)
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryMetaTag, CannotEject)
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryMetaTag, CannotSplit)
}

USTRUCT()
struct FInventoryEntryMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "InventoryEntryUserdata", meta = (Categories = "Fae.Inventory.Meta"))
	FGameplayTagContainer Tags;
};

/**
 * An extension for programmatic control over entry key permissions.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryMetadataExtension : public UInventoryReplicatedDataExtensionBase
{
	GENERATED_BODY()

protected:
	//~ UItemContainerExtensionBase
	virtual EEventExtensionResponse AllowsRemoval(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieAddress Address, FFaerieInventoryTag Reason) const override;
	//~ UItemContainerExtensionBase

	//~ UInventoryReplicatedDataExtensionBase
	virtual UScriptStruct* GetDataScriptStruct() const override;
	virtual bool SaveRepDataArray() const override { return true; }
	//~ UInventoryReplicatedDataExtensionBase

public:
	bool DoesEntryHaveTag(FFaerieAddressableHandle Handle, FFaerieInventoryMetaTag Tag) const;

	bool CanSetEntryTag(FFaerieAddressableHandle Handle, const FFaerieInventoryMetaTag Tag, const bool StateToSetTo) const;

	bool MarkStackWithTag(FFaerieAddressableHandle Handle, FFaerieInventoryMetaTag Tag);

	// @todo tag type-safety
	void TrySetTags(FFaerieAddressableHandle Handle, const FGameplayTagContainer& Tags);

	bool ClearTagFromStack(FFaerieAddressableHandle Handle, FFaerieInventoryMetaTag Tag);
};