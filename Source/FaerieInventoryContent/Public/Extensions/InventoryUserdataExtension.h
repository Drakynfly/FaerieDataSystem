// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "GameplayTagContainer.h"
#include "InventoryReplicatedDataExtensionBase.h"
#include "TypedGameplayTags.h"
#include "Actions/FaerieClientActionBase.h"
#include "InventoryUserdataExtension.generated.h"

/**
 * The key used to tag entries with custom client data.
 */
USTRUCT(BlueprintType, meta = (Categories = "Fae.Inventory.Public"))
struct FAERIEINVENTORYCONTENT_API FFaerieInventoryUserTag : public FFaerieInventoryTag
{
	GENERATED_BODY()
	END_TAG_DECL2(FFaerieInventoryUserTag, TEXT("Fae.Inventory.Public"))
};

namespace Faerie::Inventory::Tags
{
	FAERIEINVENTORYCONTENT_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryUserTag, Favorite)
}

USTRUCT()
struct FFaerieStorageEntryUserdata
{
	GENERATED_BODY()

	FFaerieStorageEntryUserdata() = default;

	FFaerieStorageEntryUserdata(const FGameplayTagContainer& Tags)
	  : Tags(Tags) {}

	UPROPERTY(EditAnywhere, Category = "InventoryEntryUserdata", meta = (Categories = "Fae.Inventory.Public"))
	FGameplayTagContainer Tags;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieStorageEntryUserdata& Other) const
	{
		return Tags == Other.Tags;
	}
};

/*
 * An extension added to player inventories that stores additional userdata about items, such as favorites.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryUserdataExtension : public UInventoryReplicatedDataExtensionBase
{
	GENERATED_BODY()

public:
	//~ UInventoryReplicatedDataExtensionBase
	virtual UScriptStruct* GetDataScriptStruct() const override;
	virtual bool SaveRepDataArray() const override { return true; }
	//~ UInventoryReplicatedDataExtensionBase

	UFUNCTION(BlueprintCallable, Category = "Faerie|UserdataExtension")
	bool DoesStackHaveTag(const UFaerieItemContainerBase* Container, FFaerieAddress Address, FFaerieInventoryUserTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|UserdataExtension")
	bool CanSetStackTag(const UFaerieItemContainerBase* Container, FFaerieAddress Address, FFaerieInventoryUserTag Tag, const bool StateToSetTo) const;

	bool MarkStackWithTag(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieAddress Address, FFaerieInventoryUserTag Tag);

	bool ClearTagFromStack(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieAddress Address, FFaerieInventoryUserTag Tag);
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MarkStackWithTag final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MarkStackWithTag")
	FFaerieItemNetworkHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "MarkStackWithTag")
	FFaerieInventoryUserTag Tag;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_ClearTagFromStack final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "ClearTagFromStack")
	FFaerieItemNetworkHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "ClearTagFromStack")
	FFaerieInventoryUserTag Tag;
};