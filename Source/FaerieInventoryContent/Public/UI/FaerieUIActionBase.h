// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieUIActionBase.generated.h"

struct FFaerieItemProxy;
class UTexture2D;
class UFaerieInventoryClient;

// Responses to actions being unable to run.
UENUM(BlueprintType)
enum class EFaerieUIActionState : uint8
{
	// Informs UI to not show this action, when it cannot run on the address
	Hidden,

	// Informs UI to show, but disable use of this action
	Disabled,

	Enabled,
};

/**
 * A UI Action is an asynchronous function class that can wrap player "actions" on items stored in inventories,
 * such as Dropping, Equipping, Consuming, Buying/Setting, etc. They usually need to send a request to the server when
 * performed by a client, so they cannot usually run in a single frame.
 */
UCLASS(Abstract, Const, Blueprintable, BlueprintType, EditInlineNew, meta = (ShowWorldContextPin))
class FAERIEINVENTORYCONTENT_API UFaerieUIActionBase : public UObject
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|UI Action")
	void Run(UFaerieInventoryClient* Client, const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|UI Action")
	EFaerieUIActionState TestCanRun(UFaerieInventoryClient* Client, const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action")
	void Finish();

	// Tries to get the local faerie client from the context.
	static UFaerieInventoryClient* GetFaerieClient(const UObject* ContextObj);

public:
	/* Gets the contextual display text for running this action on an address */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Faerie|UI Action")
	FText GetDisplayText(const FFaerieItemProxy& Proxy) const;

	/* Gets the contextual display icon for running this action on an address */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Faerie|UI Action")
	TSoftObjectPtr<UTexture2D> GetDisplayIcon(const FFaerieItemProxy& Proxy) const;

	/**
	 * Check conditions for this Action running on an Address.
	 * This is not enforced by the action when ran, it is up to the implementing UI to restrict access to the action
	 * when this returns false.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action")
	EFaerieUIActionState CanStart(const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|UI Action")
	bool Start(const FFaerieItemProxy& Proxy);

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