// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"
#include "FaerieMeshSettings.generated.h"

class AFaerieItemOwningActorBase;

/**
 *
 */
UCLASS(Config = "Project", defaultconfig)
class FAERIEITEMMESH_API UFaerieMeshSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFaerieMeshSettings();

	// UDeveloperSettings implementation
	virtual FName GetCategoryName() const override;
	// End UDeveloperSettings implementation

	// Disable to prevent UFaerieMeshSubsystem being created
	UPROPERTY(Config, EditAnywhere, Category = "Generator")
	bool CreateMeshLoaderWorldSubsystem = true;

	// If the purpose requested when loading a mesh is not available, the tag "MeshPurpose.Default" is normally used as
	// a fallback. If this is set to a tag other than that, then this will be tried first, before the default.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MeshPurpose"))
	FGameplayTag FallbackPurpose;

	// Default pick-up item actor class
	UPROPERTY(Config, EditAnywhere, Category = "Classes", meta = (ForceInlineRow))
	TSoftClassPtr<AFaerieItemOwningActorBase> DefaultPickupActor;
};