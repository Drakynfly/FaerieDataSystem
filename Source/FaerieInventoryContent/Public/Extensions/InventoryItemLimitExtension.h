// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "InventoryItemLimitExtension.generated.h"

/**
 * An inventory extension that limits the number of entries or total item count that an inventory can hold.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryItemLimitExtension : public UItemContainerExtensionBase
{
	GENERATED_BODY()

protected:
	//~ UItemContainerExtensionBase
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, TConstArrayView<FFaerieItemStackView> Views, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) override;
	//~ UItemContainerExtensionBase

public:
	// Retrieve the number of items that this inventory contains.
	UFUNCTION(BlueprintPure, Category = "Faerie|InventoryKey")
	int32 GetTotalItemCount() const;

	// Retrieve the number of entries left to be filled.
	UFUNCTION(BlueprintPure, Category = "Faerie|InventoryKey")
	int32 GetRemainingEntryCount() const;

	// Retrieve the number of items that this inventory can still contain.
	UFUNCTION(BlueprintPure, Category = "Faerie|InventoryKey")
	int32 GetRemainingTotalItemCount() const;

private:
	bool CanContain(const int32 Count) const;

	void UpdateCacheForEntry(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key);

protected:
	// Maximum number of entries the storage can contain. A value of zero doesn't apply any limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", AdvancedDisplay, meta = (ClampMin = 0))
	int32 MaxEntries = 0;

	// Maximum number of items across all stacks summed together. A value of zero doesn't apply any limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", AdvancedDisplay, meta = (ClampMin = 0))
	int32 MaxTotalItemCopies = 0;

private:
	UPROPERTY()
	TMap<FEntryKey, int32> EntryAmountCache;

	UPROPERTY()
	int32 CurrentTotalItemCopies = 0;
};