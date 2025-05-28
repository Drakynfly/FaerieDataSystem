// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "FaerieMeshSettings.generated.h"

class AItemRepresentationActor;
//class AFaerieItemOwningActorBase;

/**
 *
 */
UCLASS(Config = "Project", defaultconfig)
class FAERIEITEMMESH_API UFaerieMeshSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings implementation
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings implementation

	// Disable to prevent UFaerieMeshSubsystem being created
	UPROPERTY(Config, EditAnywhere, Category = "Generator")
	bool CreateMeshLoaderWorldSubsystem = true;

	// Default pick-up item actor class
	// @todo should be AFaerieItemOwningActorBase, but module mayhem prevents that for now...
	UPROPERTY(Config, EditAnywhere, Category = "Classes", meta = (ForceInlineRow))
	TSoftClassPtr<AItemRepresentationActor> DefaultPickupActor;
};