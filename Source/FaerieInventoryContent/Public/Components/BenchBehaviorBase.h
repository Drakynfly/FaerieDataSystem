// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "BenchBehaviorBase.generated.h"

class APlayerController;

UCLASS(Abstract, Blueprintable)
class UFaerieBenchComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BeginInteraction();

	UFUNCTION(BlueprintImplementableEvent)
	void EndInteraction();

public:
	/** WARNING: This function does not verify if this is called correctly. Only begin an interaction if you know you
	 * have permission. Usually this function would be called by an interaction manager that tracks interactability. */
	UFUNCTION(BlueprintCallable, Category = "Bench|Player Interaction")
	void NotifyInteractBegin(APlayerController* RequestingPlayer);

	/** WARNING: This function does not verify if this is called correctly. Only end an interaction if you know you
	 * have permission. Usually this function would be called by an interaction manager that tracks interactability. */
	UFUNCTION(BlueprintCallable, Category = "Bench|Player Interaction")
	void NotifyInteractEnd(APlayerController* RequestingPlayer);
};