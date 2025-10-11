// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "FaerieItemProxy.h"
#include "FaerieItemVisualBase.generated.h"

class AFaerieItemVisualBase;

namespace Faerie
{
	using FOnVisualActorDisplayFinished = TMulticastDelegate<void(bool)>;
}

/**
 * The base class for actors that visualize a faerie item.
 */
UCLASS(Abstract)
class FAERIEITEMMESH_API AFaerieItemVisualBase : public AActor, public IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	AFaerieItemVisualBase();

	//~ AActor
	virtual USceneComponent* GetDefaultAttachComponent() const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ AActor

	Faerie::FOnVisualActorDisplayFinished::RegistrationType& GetOnDisplayFinished() { return OnDisplayFinished; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemRepresentationActor")
	USceneComponent* GetDefaultMeshComponent() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|ItemRepresentationActor")
	void ClearDataDisplay();

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|ItemRepresentationActor")
	void DisplayData();

	// Function for children to call when its logic for DisplayData has finished running.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemRepresentationActor")
	void NotifyDisplayDataFinished(bool Success = true);

protected:
	void RegenerateDataDisplay();

	Faerie::FOnVisualActorDisplayFinished OnDisplayFinished;
};