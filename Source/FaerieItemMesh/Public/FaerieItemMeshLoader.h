// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "FaerieMeshStructs.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "FaerieItemMeshLoader.generated.h"

namespace Faerie::Mesh
{
	using FAsyncLoadResult = TDelegate<void(bool, FFaerieItemMesh&&)>;

	struct FAsyncLoadRequest
	{
		TWeakObjectPtr<const UObject> WeakProxy = nullptr;
		FGameplayTag Purpose;
		FAsyncLoadResult Callback;
	};

	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	FAERIEITEMMESH_API bool LoadMeshFromFragmentSynchronous(const TConstStructView<FFaerieMeshContainer> Fragment, FGameplayTag Purpose, FFaerieItemMesh& Mesh);
}

/**
 * 
 */
UCLASS()
class FAERIEITEMMESH_API UFaerieItemMeshLoader : public UObject
{
	GENERATED_BODY()

public:
	// Immediately retrieves the mesh for an item.
	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	virtual bool LoadMeshFromProxySynchronous(const FFaerieItemProxy& InProxy, FGameplayTag Purpose, FFaerieItemMesh& Mesh);

	// Asynchronously load the mesh and materials for an item.
	[[nodiscard]] TSharedPtr<FStreamableHandle> LoadMeshFromProxyAsynchronous(const FFaerieItemProxy& InProxy, FGameplayTag Purpose, Faerie::Mesh::FAsyncLoadResult Callback);

protected:
	void OnAsyncStaticMeshLoaded(TConstStructView<FFaerieStaticMeshData> MeshData, Faerie::Mesh::FAsyncLoadRequest Request);
	void OnAsyncSkeletalMeshLoaded(TConstStructView<FFaerieSkeletalMeshData> MeshData, Faerie::Mesh::FAsyncLoadRequest Request);

	virtual void HandleAsyncLoadResult(FFaerieItemMesh&& Mesh, Faerie::Mesh::FAsyncLoadRequest&& Request);
};

/**
 *
 */
USTRUCT()
struct FFaerieCachedMeshKey
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<const UObject> WeakProxy;

	UPROPERTY()
	FGameplayTag Purpose;

	bool IsKeyValid() const
	{
		return WeakProxy.IsValid();
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieCachedMeshKey& Other) const
	{
		return WeakProxy == Other.WeakProxy &&
			   Purpose == Other.Purpose;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieCachedMeshKey& Key)
	{
		return HashCombineFast(GetTypeHash(Key.WeakProxy), GetTypeHash(Key.Purpose));
	}
};

/**
 * Implementation of MeshLoader that caches results for each proxy.
 */
UCLASS()
class FAERIEITEMMESH_API UFaerieItemMeshLoader_Cached : public UFaerieItemMeshLoader
{
	GENERATED_BODY()

public:
	//~ UFaerieItemMeshLoader
	virtual bool LoadMeshFromProxySynchronous(const FFaerieItemProxy& InProxy, FGameplayTag Purpose, FFaerieItemMesh& Mesh) override;

protected:
	virtual void HandleAsyncLoadResult(FFaerieItemMesh&& Mesh, Faerie::Mesh::FAsyncLoadRequest&& Request) override;
	//~ UFaerieItemMeshLoader

public:
	// Clears all cached items.
	void ResetCache();

	// Clears the generated cache for a single proxy.
	void ResetCacheByKey(const FFaerieItemProxy& Proxy, const FGameplayTag Purpose);

private:
	/**
	 * Stored meshes for quick lookup
	 */
	UPROPERTY(Transient)
	TMap<FFaerieCachedMeshKey, FFaerieItemMesh> GeneratedMeshes;
};
