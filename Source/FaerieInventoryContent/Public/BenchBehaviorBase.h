// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "NetSupportedObject.h"

#include "BenchBehaviorBase.generated.h"

class UUserWidget;
DECLARE_DYNAMIC_DELEGATE_RetVal(bool, FBenchInteractionTest);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBenchInteractionEvent);

/**
 * A "bench" is any object that players interact with that directly interfaces with one or more inventories.
 * Primary uses are for chests, any other container, crafting/upgrading benches (origin of the name).
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories)
class FAERIEINVENTORYCONTENT_API UBenchBehaviorBase : public UActorSubobjectBase
{
	GENERATED_BODY()

	friend class ABenchVisualBase;

protected:
	// Must be called by implementing actor/component
	UFUNCTION(BlueprintImplementableEvent)
	void BeginPlay();

	UFUNCTION(BlueprintImplementableEvent)
	void BeginInteraction();

	UFUNCTION(BlueprintImplementableEvent)
	void EndInteraction();

	// When ExternalInteractionTest is not bound, this function determines interactability.
	UFUNCTION(BlueprintImplementableEvent)
	bool DefaultInteractionTest() const;

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Bench|Player Interaction")
	UUserWidget* MakeInteractionWidget(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Bench|Player Interaction")
	bool CanPlayerInteract() const;

	UFUNCTION(BlueprintCallable, Category = "Bench|Player Interaction")
	void SetInteractionTest(const FBenchInteractionTest& InteractionTest);

	/** WARNING: This function does not verify if this is called correctly. Only begin an interaction if you know you
	 * have permission. Usually this function would be called by an interaction manager that tracks interactability. */
	UFUNCTION(BlueprintCallable, Category = "Bench|Player Interaction")
	void NotifyInteractBegin(APlayerController* RequestingPlayer);

	/** WARNING: This function does not verify if this is called correctly. Only end an interaction if you know you
	 * have permission. Usually this function would be called by an interaction manager that tracks interactability. */
	UFUNCTION(BlueprintCallable, Category = "Bench|Player Interaction")
	void NotifyInteractEnd(APlayerController* RequestingPlayer);

protected:
	UPROPERTY(BlueprintAssignable, Category = "Bench|Player Interaction")
	FBenchInteractionEvent OnInteractionBegin;

	UPROPERTY(BlueprintAssignable, Category = "Bench|Player Interaction")
	FBenchInteractionEvent OnInteractionEnd;

	// A function pointer that an external source provides to determine if interaction is allowed, eg, a interaction
	// manager component on our bench.
	UPROPERTY()
	FBenchInteractionTest ExternalInteractionTest;
};