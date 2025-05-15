// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "InventoryDataStructs.h"
#include "InventoryUIAction.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryUIAction, Log, All);

class UFaerieInventoryClient;

// Responses to actions being unable to run.
UENUM(BlueprintType)
enum class EInventoryUIActionState : uint8
{
	// Informs UI to not show this action, when it cannot run on the address
	Hidden,

	// Informs UI to show, but disable use of this action
	Disabled,

	Enabled,
};

/**
 * An Inventory UI Action is an asynchronous function class that can wrap player "actions" on items stored in inventories,
 * such as Dropping, Equipping, Consuming, Buying/Setting, etc.
 */
UCLASS(Abstract, Const, Blueprintable, BlueprintType, meta = (ShowWorldContextPin))
class FAERIEINVENTORYCONTENT_API UInventoryUIAction : public UObject
{
	GENERATED_BODY()

public:
	// @todo disabled while GetFaerieClient exists, because they are incompatible. Refactor or delete this...
	//UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action", meta = (DeterminesOutput = "Class"))
	//static UInventoryUIAction2* GetActionInstance(TSubclassOf<UInventoryUIAction2> Class);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|UI Action")
	void Run(FFaerieAddressableHandle Handle) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action")
	void Finish();

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|UI Action", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool GetFaerieClient(UFaerieInventoryClient*& Client) const;

public:
	/* Gets the contextual display text for running this action on an address */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Faerie|UI Action")
	FText GetDisplayText(FFaerieAddressableHandle Handle) const;

	/* Gets the contextual display icon for running this action on an address */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Faerie|UI Action")
	TSoftObjectPtr<UTexture2D> GetDisplayIcon(FFaerieAddressableHandle Handle) const;

	/**
	 * Check conditions for this Action running on an Address.
	 * This is not enforced by the action when ran, it is up to the implementing UI to restrict access to the action
	 * when this returns false.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Faerie|UI Action")
	EInventoryUIActionState TestCanRun(FFaerieAddressableHandle Handle) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action")
	bool Start(FFaerieAddressableHandle Handle);

protected:
	/** Text to display on a user-facing button */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Config")
	FText ButtonLabel;

	/** Icon to display on a user-facing button */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Config")
	TSoftObjectPtr<UTexture2D> ButtonIcon;

private:
	bool InProgress = false;
};