// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "InventoryDataStructs.h"
#include "InventoryUIAction.generated.h"

class UFaerieItemContainerBase;
// Responses to actions being unable to run.
UENUM(BlueprintType)
enum class EInventoryUIActionState : uint8
{
	// Informs UI to not show this action, when it cannot run on the entry
	Hidden,

	// Informs UI to show, but disable use of this action
	Disabled,

	Enabled,
};

DECLARE_LOG_CATEGORY_EXTERN(LogInventoryUIAction, Log, All);

class UInventoryContentsBase;

/**
 *
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class FAERIEINVENTORYCONTENT_API UInventoryUIAction : public UObject
{
	GENERATED_BODY()

protected:
	virtual UWorld* GetWorld() const override;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "UI Action")
	void Run();

	UFUNCTION(BlueprintCallable, Category = "UI Action")
	virtual void Finish();

public:
	/**
	 * Check conditions for this Action running on an Entry. This is not enforced by the action when ran, it is up to
	 * the implementing UI to restrict access to the action when this returns false.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UI Action")
	EInventoryUIActionState CanRunOnEntry(FInventoryKey InKey) const;

	/**
	 * This must be called with the targeting inventory prior to any call to Start.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UI Action")
	void Setup(UInventoryContentsBase* InContentsWidget);

	UFUNCTION(BlueprintCallable, Category = "UI Action")
	bool Start(FInventoryKey InKey);

protected:
	/** Text to display on user-facing button */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "UI Action")
	FText ButtonLabel;

	/** Icon to display on user-facing button */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "UI Action")
	TSoftObjectPtr<UTexture2D> ButtonIcon;

	UPROPERTY(BlueprintReadOnly, Category = "UI Action")
	TObjectPtr<UInventoryContentsBase> ContentsWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI Action")
	FInventoryKey Key;

private:
	bool InProgress = false;
};

/**
 * Second iteration of UI Actions
 */
UCLASS(Abstract, Const, Blueprintable, BlueprintType)
class FAERIEINVENTORYCONTENT_API UInventoryUIAction2 : public UObject
{
	GENERATED_BODY()

protected:
	//virtual UWorld* GetWorld() const override;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "UI Action")
	void Run() const;

	UFUNCTION(BlueprintCallable, Category = "UI Action")
	void Finish();

public:
	/**
	 * Check conditions for this Action running on a Proxy.
	 * This is not enforced by the action when ran, it is up to the implementing UI to restrict access to the action
	 * when this returns false.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UI Action")
	EInventoryUIActionState CanRunOnProxy(UFaerieItemContainerBase* InContainer, FEntryKey InKey) const;

	UFUNCTION(BlueprintCallable, Category = "UI Action")
	bool Start(UFaerieItemContainerBase* InContainer, FEntryKey InKey);

protected:
	/** Text to display on user-facing button */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "UI Action")
	FText ButtonLabel;

	/** Icon to display on user-facing button */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "UI Action")
	TSoftObjectPtr<UTexture2D> ButtonIcon;

	UPROPERTY(BlueprintReadOnly, Category = "UI Action")
	TObjectPtr<UFaerieItemContainerBase> Container;

	UPROPERTY(BlueprintReadOnly, Category = "UI Action")
	FEntryKey Key;

private:
	bool InProgress = false;
};