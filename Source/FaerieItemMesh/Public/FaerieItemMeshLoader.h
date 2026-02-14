// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "FaerieMeshStructs.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "FaerieItemMeshLoader.generated.h"

struct FFaerieDynamicSkeletalMesh;
struct FFaerieDynamicStaticMesh;
class UFaerieMeshTokenBase;

namespace Faerie
{
	using FItemMeshAsyncLoadResult = TDelegate<void(bool, FFaerieItemMesh&&)>;

	struct FAsyncLoadRequest
	{
		TWeakObjectPtr<const UFaerieMeshTokenBase> Token = nullptr;
		FGameplayTag Purpose;
		FItemMeshAsyncLoadResult Callback;
	};

	FAERIEITEMMESH_API FFaerieItemMesh GetDynamicStaticMeshForData(const FFaerieDynamicStaticMesh& MeshData);

	FAERIEITEMMESH_API FFaerieItemMesh GetDynamicSkeletalMeshForData(const FFaerieDynamicSkeletalMesh& MeshData);

	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	FAERIEITEMMESH_API bool LoadMeshFromTokenSynchronous(TNotNull<const UFaerieMeshTokenBase*> Token, FGameplayTag Purpose, FFaerieItemMesh& Mesh);

	// Immediately retrieves the mesh for an item.
	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	FAERIEITEMMESH_API bool LoadMeshFromProxySynchronous(FFaerieItemProxy Proxy, FGameplayTag Purpose, FFaerieItemMesh& Mesh);
}

/**
 * 
 */
UCLASS()
class FAERIEITEMMESH_API UFaerieItemMeshLoader : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem")
	static FFaerieItemMesh GetDynamicStaticMeshForData(const FFaerieDynamicStaticMesh& MeshData);

	UFUNCTION(BlueprintCallable, Category = "Faerie|MeshSubsystem")
	static FFaerieItemMesh GetDynamicSkeletalMeshForData(const FFaerieDynamicSkeletalMesh& MeshData);

	// Immediately retrieves the mesh for an item.
	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	virtual bool LoadMeshFromTokenSynchronous(TNotNull<const UFaerieMeshTokenBase*> Token, FGameplayTag Purpose, FFaerieItemMesh& Mesh);

	// Immediately retrieves the mesh for an item.
	// WARNING: This can cause a hitch if the mesh is not cached, and it requires a lengthy load or assembly.
	virtual bool LoadMeshFromProxySynchronous(FFaerieItemProxy Proxy, FGameplayTag Purpose, FFaerieItemMesh& Mesh);

	// Asynchronously load the mesh and materials for an item.
	[[nodiscard]] TSharedPtr<FStreamableHandle> LoadMeshFromTokenAsynchronous(TNotNull<const UFaerieMeshTokenBase*> Token, FGameplayTag Purpose, Faerie::FItemMeshAsyncLoadResult Callback);

	// Asynchronously load the mesh and materials for an item.
	[[nodiscard]] TSharedPtr<FStreamableHandle> LoadMeshFromProxyAsynchronous(FFaerieItemProxy Proxy, FGameplayTag Purpose, Faerie::FItemMeshAsyncLoadResult Callback);

protected:
	void OnAsyncStaticMeshLoaded(TConstStructView<FFaerieStaticMeshData> MeshData, Faerie::FAsyncLoadRequest Request);
	void OnAsyncSkeletalMeshLoaded(TConstStructView<FFaerieSkeletalMeshData> MeshData, Faerie::FAsyncLoadRequest Request);
	void OnAsyncDynamicStaticMeshLoaded(TConstStructView<FFaerieDynamicStaticMesh> MeshData, Faerie::FAsyncLoadRequest Request);
	void OnAsyncDynamicSkeletalMeshLoaded(TConstStructView<FFaerieDynamicSkeletalMesh> MeshData, Faerie::FAsyncLoadRequest Request);

	virtual void HandleAsyncLoadResult(FFaerieItemMesh&& Mesh, Faerie::FAsyncLoadRequest&& Request);
};

/**
 *
 */
USTRUCT()
struct FFaerieCachedMeshKey
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<const UFaerieMeshTokenBase> Token;

	UPROPERTY()
	FGameplayTag Purpose;

	bool IsTokenValid() const
	{
		return Token.IsValid();
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieCachedMeshKey& Other) const
	{
		return Token == Other.Token &&
			   Purpose == Other.Purpose;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieCachedMeshKey& Key)
	{
		return HashCombineFast(GetTypeHash(Key.Token), GetTypeHash(Key.Purpose));
	}
};

/**
 * Implementation of MeshLoader that caches results for each token instance.
 */
UCLASS()
class FAERIEITEMMESH_API UFaerieItemMeshLoader_Cached : public UFaerieItemMeshLoader
{
	GENERATED_BODY()

public:
	//~ UFaerieItemMeshLoader
	virtual bool LoadMeshFromTokenSynchronous(TNotNull<const UFaerieMeshTokenBase*> Token, const FGameplayTag Purpose, FFaerieItemMesh& Mesh) override;
	//virtual bool LoadMeshFromProxySynchronous(FFaerieItemProxy Proxy, const FGameplayTag Purpose, FFaerieItemMesh& Mesh) override;

protected:
	virtual void HandleAsyncLoadResult(FFaerieItemMesh&& Mesh, Faerie::FAsyncLoadRequest&& Request) override;
	//~ UFaerieItemMeshLoader

public:
	// Clears all cached items.
	void ResetCache();

	// Clears the generated cache for a single token.
	void ResetCacheByKey(const UFaerieMeshTokenBase* Token, const FGameplayTag Purpose);

private:
	/**
	 * Stored meshes for quick lookup
	 */
	UPROPERTY(Transient)
	TMap<FFaerieCachedMeshKey, FFaerieItemMesh> GeneratedMeshes;
};
