// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "GameFramework/Actor.h"
#include "ItemRepresentationActor.generated.h"

class UFaerieItemMeshLoader;

/**
 * The base actor class for physical representations of inventory entries.
 */
UCLASS(Abstract)
class FAERIEITEMMESH_API AItemRepresentationActor : public AActor
{
	GENERATED_BODY()

	friend class FFaerieItemAssetPreviewScene;

public:
	AItemRepresentationActor();

	virtual void Destroyed() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|ItemRepresentationActor")
	void ClearDataDisplay();

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|ItemRepresentationActor")
	void DisplayData();

private:
	void RegenerateDataDisplay();

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemRepresentationActor")
	void SetSourceProxy(FFaerieItemProxy Source);

protected:
	// The wrapper for the data we are going to display. By keeping the data abstracted behind a FaerieItemProxy,
	// this allows AItemRepresentationActor to display data from an Inventory, or standalone data, etc, just as well,
	// with the same API.
	// Proxies typically cannot replicate. If a particular child wants to replicate some or all of the data, it
	// needs to extract out the data it needs into a separate replicated variable.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FFaerieItemProxy DataSource = nullptr;
};