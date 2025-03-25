﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "FaerieMeshStructs.h"
#include "Subsystems/WorldSubsystem.h"
#include "FaerieMeshSubsystem.generated.h"

struct FFaerieItemProxy;
class UFaerieItemMeshLoader;
class UFaerieMeshTokenBase;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FFaerieItemMeshAsyncLoadResult, bool, Success, const FFaerieItemMesh&, Mesh);

/**
 * This is a world subsystem that stores dynamically generated meshes for items.
 */
UCLASS()
class FAERIEITEMMESH_API UFaerieMeshSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	// Immediately retrieves the mesh for an item.
	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	// Use the Asynchronous version if possible to avoid this.
	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool LoadMeshFromTokenSynchronous(const UFaerieMeshTokenBase* Token,
		UPARAM(meta = (Categories = "MeshPurpose")) FGameplayTag Purpose, FFaerieItemMesh& Mesh);

	// Immediately retrieves the mesh for an item.
	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	// Use the Asynchronous version if possible to avoid this.
	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool LoadMeshFromProxySynchronous(FFaerieItemProxy Proxy,
		UPARAM(meta = (Categories = "MeshPurpose")) FGameplayTag Purpose, FFaerieItemMesh& Mesh);

	// Asynchronously load the mesh and materials for an item.
	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem")
	void LoadMeshFromTokenAsynchronous(const UFaerieMeshTokenBase* Token,
		UPARAM(meta = (Categories = "MeshPurpose")) FGameplayTag Purpose, const FFaerieItemMeshAsyncLoadResult& Callback);

	// Asynchronously load the mesh and materials for an item.
	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem")
	void LoadMeshFromProxyAsynchronous(FFaerieItemProxy Proxy,
		UPARAM(meta = (Categories = "MeshPurpose")) FGameplayTag Purpose, const FFaerieItemMeshAsyncLoadResult& Callback);

protected:
	// If the purpose requested when loading a mesh is not available, the tag "MeshPurpose.Default" is normally used as
	// a fallback. If this is set to a tag other than that, then this will be tried first, before the default.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MeshPurpose"))
	FGameplayTag FallbackPurpose;

	UPROPERTY()
	TObjectPtr<UFaerieItemMeshLoader> Loader;
};