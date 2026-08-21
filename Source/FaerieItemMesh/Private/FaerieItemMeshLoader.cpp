// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemMeshLoader.h"
#include "FaerieItem.h"

#include "EntityManagerHelpers.h"
#include "FaerieItemMeshLog.h"
#include "FaerieItemProxy.h"

#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"

#include "Fragments/FaerieMeshFragment.h"

#include "Materials/MaterialInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemMeshLoader)

using namespace Faerie;

namespace Faerie::Mesh
{
	bool LoadMeshFromFragmentSynchronous(const TConstStructView<FFaerieMeshContainer> Fragment, const FGameplayTag Purpose, FFaerieItemMesh& Mesh)
	{
		FGameplayTagContainer PurposeHierarchy;
		if (Purpose != Tags::MeshPurpose_Default)
		{
			PurposeHierarchy.AddTagFast(Purpose);
		}
		PurposeHierarchy.AddTagFast(Tags::MeshPurpose_Default);

		// Otherwise, scan and load pre-defined mesh data.

		if (const TConstStructView<FFaerieSkeletalMeshData> SkelMeshData = Fragment->GetSkeletalItemMesh(PurposeHierarchy);
			SkelMeshData.IsValid())
		{
			Mesh = FFaerieItemMesh::MakeSkeletal(SkelMeshData.Get());
			return true;
		}

		if (const TConstStructView<FFaerieStaticMeshData> StaticMeshData = Fragment->GetStaticItemMesh(PurposeHierarchy);
			StaticMeshData.IsValid())
		{
			Mesh = FFaerieItemMesh::MakeStatic(StaticMeshData.Get());
			return true;
		}

		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Asset does not contain a mesh suitable for the purpose.", __FUNCTION__)
		return false;
	}
}

bool UFaerieItemMeshLoader::LoadMeshFromProxySynchronous(const FFaerieItemProxy& InProxy, const FGameplayTag Purpose,
														 FFaerieItemMesh& Mesh)
{
	if (!InProxy.IsValid())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid proxy!", __FUNCTION__)
		return false;
	}

	const TOptional<FFaerieItemInstance> Instance = InProxy.GetItemInstance();
	if (!Instance.IsSet())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid instance!", __FUNCTION__)

		return false;
	}

	auto* EntityManager = ItemData::GetFaerieEntityManager();
	auto MeshFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieMeshFragment>(EntityManager, Instance.GetValue());
	if (!MeshFragment.IsValid())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid fragment!", __FUNCTION__)
		return false;
	}

	return Mesh::LoadMeshFromFragmentSynchronous(MeshFragment->Container, Purpose, Mesh);
}

TSharedPtr<FStreamableHandle> UFaerieItemMeshLoader::LoadMeshFromProxyAsynchronous(const FFaerieItemProxy& InProxy,
	const FGameplayTag Purpose, Mesh::FAsyncLoadResult Callback)
{
	if (!InProxy.IsValid())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid proxy!", __FUNCTION__)
		(void)Callback.ExecuteIfBound(false, {});
		return nullptr;
	}

	const TOptional<FFaerieItemInstance> Instance = InProxy.GetItemInstance();
	if (!Instance.IsSet())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid instance!", __FUNCTION__)
		(void)Callback.ExecuteIfBound(false, {});
		return nullptr;
	}

	auto* EntityManager = ItemData::GetFaerieEntityManager();
	auto MeshFragment = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieMeshFragment>(EntityManager, Instance.GetValue());
	if (!MeshFragment.IsValid())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid fragment!", __FUNCTION__)
		(void)Callback.ExecuteIfBound(false, {});
		return nullptr;
	}

	Mesh::FAsyncLoadRequest LoadRequest;
	LoadRequest.WeakProxy = InProxy.GetProxyObject();
	LoadRequest.Purpose = Purpose;
	LoadRequest.Callback = MoveTemp(Callback);

	FGameplayTagContainer PurposeHierarchy;
	if (Purpose.IsValid() && Purpose != Mesh::Tags::MeshPurpose_Default)
	{
		if (ensure(Purpose.GetTagName().IsValid()))
		{
			PurposeHierarchy.AddTagFast(Purpose);
		}
	}
	PurposeHierarchy.AddTagFast(Mesh::Tags::MeshPurpose_Default);

	// Otherwise, scan and load pre-defined mesh data.

	if (const TConstStructView<FFaerieSkeletalMeshData> SkelMeshData = MeshFragment->Container.GetSkeletalItemMesh(PurposeHierarchy);
		SkelMeshData.IsValid())
	{
		const FFaerieSkeletalMeshData& MeshData = SkelMeshData.Get();
		TArray<FSoftObjectPath> AssetsToLoad;
		if (MeshData.SkeletonAndAnimation.Mesh.IsPending())
		{
			AssetsToLoad.Add(MeshData.SkeletonAndAnimation.Mesh.ToSoftObjectPath());
		}
		if (MeshData.SkeletonAndAnimation.AnimClass.IsPending())
		{
			AssetsToLoad.Add(MeshData.SkeletonAndAnimation.AnimClass.ToSoftObjectPath());
		}
		for (auto&& Material : MeshData.Materials)
		{
			if (Material.Material.IsPending())
			{
				AssetsToLoad.Add(Material.Material.ToSoftObjectPath());
			}
		}

		if (AssetsToLoad.IsEmpty())
		{
			OnAsyncSkeletalMeshLoaded(SkelMeshData, MoveTemp(LoadRequest));
		}
		else
		{
			return UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(AssetsToLoad),
				FStreamableDelegate::CreateUObject(this, &ThisClass::OnAsyncSkeletalMeshLoaded, SkelMeshData, LoadRequest));
		}
		return nullptr;
	}

	if (const TConstStructView<FFaerieStaticMeshData> StaticMeshData = MeshFragment->Container.GetStaticItemMesh(PurposeHierarchy);
		StaticMeshData.IsValid())
	{
		const FFaerieStaticMeshData& MeshData = StaticMeshData.Get();
		TArray<FSoftObjectPath> AssetsToLoad;
		if (MeshData.StaticMesh.IsPending())
		{
			AssetsToLoad.Add(MeshData.StaticMesh.ToSoftObjectPath());
		}
		for (auto&& Material : MeshData.Materials)
		{
			if (Material.Material.IsPending())
			{
				AssetsToLoad.Add(Material.Material.ToSoftObjectPath());
			}
		}

		if (AssetsToLoad.IsEmpty())
		{
			OnAsyncStaticMeshLoaded(StaticMeshData, MoveTemp(LoadRequest));
		}
		else
		{
			return UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(AssetsToLoad),
				FStreamableDelegate::CreateUObject(this, &ThisClass::OnAsyncStaticMeshLoaded, StaticMeshData, LoadRequest));
		}
		return nullptr;
	}

	UE_LOGF(LogFaerieItemMesh, Error, "%hs: Asset does not contain a mesh suitable for the purpose.", __FUNCTION__)
	(void)Callback.ExecuteIfBound(false, {});
	return nullptr;
}

void UFaerieItemMeshLoader::OnAsyncStaticMeshLoaded(const TConstStructView<FFaerieStaticMeshData> MeshData,
	Mesh::FAsyncLoadRequest Request)
{
	FFaerieItemMesh Mesh = FFaerieItemMesh::MakeStatic(MeshData.Get());
	HandleAsyncLoadResult(MoveTemp(Mesh), MoveTemp(Request));
}

void UFaerieItemMeshLoader::OnAsyncSkeletalMeshLoaded(const TConstStructView<FFaerieSkeletalMeshData> MeshData,
	Mesh::FAsyncLoadRequest Request)
{
	FFaerieItemMesh Mesh = FFaerieItemMesh::MakeSkeletal(MeshData.Get());
	HandleAsyncLoadResult(MoveTemp(Mesh), MoveTemp(Request));
}

void UFaerieItemMeshLoader::HandleAsyncLoadResult(FFaerieItemMesh&& Mesh, Mesh::FAsyncLoadRequest&& Request)
{
	(void)Request.Callback.ExecuteIfBound(true, MoveTemp(Mesh));
}

bool UFaerieItemMeshLoader_Cached::LoadMeshFromProxySynchronous(const FFaerieItemProxy& InProxy, const FGameplayTag Purpose,
	FFaerieItemMesh& Mesh)
{
	const FFaerieCachedMeshKey Key = {InProxy.GetProxyObject(), Purpose};

	// If we have already generated this mesh, return that one.
	if (auto&& CachedMesh = GeneratedMeshes.Find(Key))
	{
		Mesh = *CachedMesh;
		return true;
	}

	const bool SuperResult = Super::LoadMeshFromProxySynchronous(InProxy, Purpose, Mesh);

	// If the mesh load succeeded, cache the result.
	if (SuperResult)
	{
		GeneratedMeshes.Add(Key, Mesh);
	}

	return SuperResult;
}

void UFaerieItemMeshLoader_Cached::HandleAsyncLoadResult(FFaerieItemMesh&& Mesh, Mesh::FAsyncLoadRequest&& Request)
{
	const FFaerieCachedMeshKey Key = {Request.WeakProxy, Request.Purpose};
	GeneratedMeshes.Add(Key, Mesh);
	Super::HandleAsyncLoadResult(MoveTemp(Mesh), MoveTemp(Request));
}

void UFaerieItemMeshLoader_Cached::ResetCache()
{
	GeneratedMeshes.Reset();
}

void UFaerieItemMeshLoader_Cached::ResetCacheByKey(const FFaerieItemProxy& Proxy, const FGameplayTag Purpose)
{
	const FFaerieCachedMeshKey Key = {Proxy.GetProxyObject(), Purpose};
	GeneratedMeshes.Remove(Key);
}
