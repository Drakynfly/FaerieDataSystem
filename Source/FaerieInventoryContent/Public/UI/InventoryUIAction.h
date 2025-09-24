// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "InventoryUIAction.generated.h"

class UTexture2D;
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
UCLASS(Abstract, Const, Blueprintable, BlueprintType, EditInlineNew, meta = (ShowWorldContextPin))
class FAERIEINVENTORYCONTENT_API UInventoryUIAction : public UObject
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|UI Action")
	void Run(UFaerieInventoryClient* Client, FFaerieAddressableHandle Handle) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|UI Action")
	EInventoryUIActionState TestCanRun(UFaerieInventoryClient* Client, FFaerieAddressableHandle Handle) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action")
	void Finish();

	// Tries to get the local faerie client from the context.
	static UFaerieInventoryClient* GetFaerieClient(const UObject* ContextObj);

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
	UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action")
	EInventoryUIActionState CanStart(FFaerieAddressableHandle Handle) const;

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