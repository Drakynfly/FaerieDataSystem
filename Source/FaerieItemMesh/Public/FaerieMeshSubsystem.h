// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "FaerieMeshStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "FaerieMeshSubsystem.generated.h"

namespace Faerie::ItemData
{
	struct FReference;
}

struct FFaerieItemInstance;
class UFaerieItemMeshLoader;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FFaerieItemMeshAsyncLoadResult, bool, Success, const FFaerieItemMesh&, Mesh);

/**
 * This is a world subsystem that stores dynamically generated meshes for items.
 */
UCLASS()
class FAERIEITEMMESH_API UFaerieMeshSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	// Immediately retrieves the mesh for an item.
	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	// Use the Asynchronous version if possible to avoid this.
	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool LoadMeshFromProxySynchronous(const FFaerieItemProxy& Proxy,
		UPARAM(meta = (Categories = "MeshPurpose")) FGameplayTag Purpose, FFaerieItemMesh& Mesh);

	// Asynchronously load the mesh and materials for an item.
	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem")
	void LoadMeshFromProxyAsynchronous(const FFaerieItemProxy& Proxy,
		UPARAM(meta = (Categories = "MeshPurpose")) FGameplayTag Purpose, const FFaerieItemMeshAsyncLoadResult& Callback);
	[[nodiscard]] TSharedPtr<FStreamableHandle> LoadMeshFromProxyAsynchronous(const FFaerieItemProxy& Proxy, FGameplayTag Purpose, const TDelegate<void(bool, FFaerieItemMesh&&)>& Callback);

protected:
	UPROPERTY()
	TObjectPtr<UFaerieItemMeshLoader> Loader;
};