// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "ItemContainerExtensionBase.h"
#include "InventoryReplicatedDataExtensionBase.h"
#include "TypedGameplayTags.h"
#include "Actions/FaerieInventoryClient.h"
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
struct FInventoryEntryUserdata
{
	GENERATED_BODY()

	FInventoryEntryUserdata() = default;

	FInventoryEntryUserdata(const FGameplayTagContainer& Tags)
	  : Tags(Tags) {}

	UPROPERTY(EditAnywhere, Category = "InventoryEntryUserdata", meta = (Categories = "Fae.Inventory.Public"))
	FGameplayTagContainer Tags;

	friend bool operator==(const FInventoryEntryUserdata& Lhs, const FInventoryEntryUserdata& Rhs)
	{
		return Lhs.Tags == Rhs.Tags;
	}

	friend bool operator!=(const FInventoryEntryUserdata& Lhs, const FInventoryEntryUserdata& Rhs)
	{
		return !(Lhs == Rhs);
	}
};

/*
 * An extension added to player inventories that stores additional userdata about items, such as favorites.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryUserdataExtension : public UInventoryReplicatedDataExtensionBase
{
	GENERATED_BODY()

protected:
	//~ UInventoryReplicatedDataExtensionBase
	virtual UScriptStruct* GetDataScriptStruct() const override;
	virtual bool SaveRepDataArray() const override { return true; }
	//~ UInventoryReplicatedDataExtensionBase

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|UserdataExtension")
	bool DoesStackHaveTag(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryUserTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|UserdataExtension")
	bool CanSetStackTag(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryUserTag Tag, const bool StateToSetTo) const;

	bool MarkStackWithTag(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryUserTag Tag);

	bool ClearTagFromStack(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryUserTag Tag);
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MarkStackWithTag final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MarkStackWithTag")
	FContainerEntryHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "MarkStackWithTag")
	FFaerieInventoryUserTag Tag;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_ClearTagFromStack final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "ClearTagFromStack")
	FContainerEntryHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "ClearTagFromStack")
	FFaerieInventoryUserTag Tag;
};