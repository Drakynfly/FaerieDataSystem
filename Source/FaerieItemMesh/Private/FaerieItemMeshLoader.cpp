// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemMeshLoader.h"
#include "FaerieItem.h"

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

bool UFaerieItemMeshLoader::LoadMeshFromProxySynchronous(const FMassEntityManager& EntityManager, const FFaerieItemProxy& InProxy, const FGameplayTag Purpose,
														 FFaerieItemMesh& Mesh)
{
	const TOptional<FFaerieItemInstance> Instance = InProxy.GetItemInstance();
	if (!Instance.IsSet())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid proxy/instance!", __FUNCTION__)

		return false;
	}

	auto MeshFragment = ItemData::GetEntityFragmentOrDefault<FFaerieMeshFragment>(&EntityManager, Instance.GetValue());
	if (!MeshFragment.IsValid())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid fragment!", __FUNCTION__)
		return false;
	}

	return Mesh::LoadMeshFromFragmentSynchronous(MeshFragment->Container, Purpose, Mesh);
}

TSharedPtr<FStreamableHandle> UFaerieItemMeshLoader::LoadMeshFromProxyAsynchronous(const FMassEntityManager& EntityManager, const FFaerieItemProxy& InProxy,
	const FGameplayTag Purpose, Mesh::FAsyncLoadResult Callback)
{
	const TOptional<FFaerieItemInstance> Instance = InProxy.GetItemInstance();
	if (!Instance.IsSet())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid proxy/instance!", __FUNCTION__)
		(void)Callback.ExecuteIfBound(false, {});
		return nullptr;
	}

	auto MeshFragment = ItemData::GetEntityFragmentOrDefault<FFaerieMeshFragment>(&EntityManager, Instance.GetValue());
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

#if WITH_EDITOR
bool UFaerieItemMeshLoader::LoadMeshFromAssetSynchronous(const TNotNull<const UFaerieItem*> InItem, const FGameplayTag Purpose,
	FFaerieItemMesh& Mesh)
{
	if (!IsValid(InItem))
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid proxy!", __FUNCTION__)
		return false;
	}

	const TConstStructView<FFaerieMeshFragment> MeshFragment = InItem->GetDefaultFragment<FFaerieMeshFragment>();
	if (!MeshFragment.IsValid())
	{
		UE_LOGF(LogFaerieItemMesh, Error, "%hs: Invalid fragment!", __FUNCTION__)
		return false;
	}

	return Mesh::LoadMeshFromFragmentSynchronous(MeshFragment->Container, Purpose, Mesh);
}
#endif

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

bool UFaerieItemMeshLoader_Cached::LoadMeshFromProxySynchronous(const FMassEntityManager& EntityManager, const FFaerieItemProxy& InProxy, const FGameplayTag Purpose,
	FFaerieItemMesh& Mesh)
{
	const Mesh::FCachedMeshKey Key = {
		.WeakProxy = InProxy.GetProxyObject(),
		.Purpose = Purpose
	};

	// If we have already generated this mesh, return that one.
	if (auto&& CachedMesh = GeneratedMeshes.Find(Key))
	{
		Mesh = *CachedMesh;
		return true;
	}

	const bool SuperResult = Super::LoadMeshFromProxySynchronous(EntityManager, InProxy, Purpose, Mesh);

	// If the mesh load succeeded, cache the result.
	if (SuperResult)
	{
		GeneratedMeshes.Add(Key, Mesh);
	}

	return SuperResult;
}

void UFaerieItemMeshLoader_Cached::HandleAsyncLoadResult(FFaerieItemMesh&& Mesh, Mesh::FAsyncLoadRequest&& Request)
{
	const Mesh::FCachedMeshKey Key = {
		.WeakProxy = Request.WeakProxy,
		.Purpose = Request.Purpose
	};
	GeneratedMeshes.Add(Key, Mesh);
	Super::HandleAsyncLoadResult(MoveTemp(Mesh), MoveTemp(Request));
}

void UFaerieItemMeshLoader_Cached::ResetCache()
{
	GeneratedMeshes.Reset();
}

void UFaerieItemMeshLoader_Cached::ResetCacheByKey(const FFaerieItemProxy& Proxy, const FGameplayTag Purpose)
{
	const Mesh::FCachedMeshKey Key = {
		.WeakProxy = Proxy.GetProxyObject(),
		.Purpose = Purpose
	};
	GeneratedMeshes.Remove(Key);
}
