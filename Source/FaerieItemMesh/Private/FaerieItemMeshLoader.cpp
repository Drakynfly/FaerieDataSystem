// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemMeshLoader.h"
#include "FaerieItem.h"
#include "SkeletalMergingLibrary.h"
#include "Tokens/FaerieMeshToken.h"

#include "Engine/StaticMeshSocket.h"
#//include "Engine/SkeletalMeshSocket.h"

#include "FaerieItemMeshLog.h"
#include "UDynamicMesh.h" // For creating static meshes at runtime
#include "Engine/AssetManager.h"

#include "GeometryScript/MeshAssetFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"

namespace Faerie
{
	FFaerieItemMesh GetDynamicStaticMeshForData(const FFaerieDynamicStaticMesh& MeshData)
	{
		if (MeshData.Fragments.IsEmpty())
		{
			return FFaerieItemMesh();
		}

		// The final mesh we will return.
		UDynamicMesh* OutMesh = NewObject<UDynamicMesh>();
		TArray<FFaerieItemMaterial> Materials;

		TMap<FName, UStaticMeshSocket*> Sockets;

		for (const FFaerieDynamicStaticMeshFragment& Fragment : MeshData.Fragments)
		{
			if (!Fragment.StaticMesh.IsValid())
			{
				UE_LOG(LogFaerieItemMesh, Error, TEXT("%hs: Invalid Static Mesh detected while building dynamic mesh!"), __FUNCTION__)
				continue;
			}

			// Copy mesh data

			UDynamicMesh* AppendMesh = NewObject<UDynamicMesh>();

			const FGeometryScriptCopyMeshFromAssetOptions AssetOptions;
			const FGeometryScriptMeshReadLOD RequestedLOD;
			EGeometryScriptOutcomePins Outcome;
			UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromStaticMesh(Fragment.StaticMesh.LoadSynchronous(), AppendMesh, AssetOptions, RequestedLOD, Outcome);

			for (auto&& Socket : Fragment.StaticMesh->Sockets)
			{
				Sockets.Add(Socket->SocketName, Socket);
			}

			// Figure out mesh transform

			FTransform AppendTransform = Fragment.Attachment.Offset;

			if (!Fragment.Attachment.Socket.IsNone())
			{
				if (auto&& Socket = Sockets.Find(Fragment.Attachment.Socket);
					Socket && IsValid(*Socket))
				{
					AppendTransform *= FTransform((*Socket)->RelativeRotation, (*Socket)->RelativeLocation, (*Socket)->RelativeScale);
				}
			}

			// Align material IDs

			const int32 NumStaticMaterials = Fragment.StaticMesh->GetStaticMaterials().Num();
			const int32 NumDynamicMaterials = Fragment.Materials.Num();
			const int32 MaterialOverrideNum = FMath::Min(NumStaticMaterials, NumDynamicMaterials);

			for (int32 MatOverrideIndex = 0; MatOverrideIndex < MaterialOverrideNum; ++MatOverrideIndex)
			{
				if (const int32 ExistingIndex = Materials.IndexOfByPredicate(
					[&](const FFaerieItemMaterial& IndexedMat)
					{
						return IndexedMat.Material == Fragment.StaticMesh->GetMaterial(MatOverrideIndex);
					});
					ExistingIndex != INDEX_NONE)
				{
					UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(AppendMesh, MatOverrideIndex, ExistingIndex);
				}
				else
				{
					const int32 NewIndex = Materials.Emplace(Fragment.Materials[MatOverrideIndex].Material.LoadSynchronous());
					UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(AppendMesh, MatOverrideIndex, NewIndex);
				}
			}

			// Commit new mesh
			UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(OutMesh, AppendMesh, AppendTransform);

			// Trash temp dynamic mesh.
			AppendMesh->MarkAsGarbage();
		}

		return FFaerieItemMesh::MakeDynamic(OutMesh, Materials);
	}

	FFaerieItemMesh GetDynamicSkeletalMeshForData(const FFaerieDynamicSkeletalMesh& MeshData)
	{
		// @todo implement
		UE_LOG(LogFaerieItemMesh, Warning, TEXT("%hs: GetDynamicSkeletalMeshForData is not implemented."), __FUNCTION__)

		FSkeletonAndAnimation OutSkeletonAndAnimation;
		TArray<FFaerieItemMaterial> Materials;

		FSkeletalMeshMergeParams Params;
		OutSkeletonAndAnimation.Mesh = USkeletalMergingLibrary::MergeMeshes(Params);

		return FFaerieItemMesh::MakeSkeletal(OutSkeletonAndAnimation, Materials);
	}

	bool LoadMeshFromTokenSynchronous(const UFaerieMeshTokenBase* Token, const FGameplayTag Purpose, FFaerieItemMesh& Mesh)
	{
		if (!IsValid(Token))
		{
			UE_LOG(LogFaerieItemMesh, Warning, TEXT("%hs: No MeshToken on entry"), __FUNCTION__)
			return false;
		}

		FGameplayTagContainer PurposeHierarchy;
		if (Purpose != ItemMesh::Tags::MeshPurpose_Default)
		{
			PurposeHierarchy.AddTagFast(Purpose);
		}
		PurposeHierarchy.AddTagFast(ItemMesh::Tags::MeshPurpose_Default);

		// Check for the presence of a custom dynamic mesh to build.

		if (auto&& DynamicMeshToken = Cast<UFaerieMeshToken_Dynamic>(Token))
		{
			if (auto&& SkeletalMesh = DynamicMeshToken->GetDynamicSkeletalItemMesh(PurposeHierarchy);
				SkeletalMesh.IsValid())
			{
				Mesh = GetDynamicSkeletalMeshForData(SkeletalMesh.Get());
				if (Mesh.IsSkeletal())
				{
					return true;
				}
			}

			if (auto&& StaticMesh = DynamicMeshToken->GetDynamicStaticItemMesh(PurposeHierarchy);
				StaticMesh.IsValid())
			{
				Mesh = GetDynamicStaticMeshForData(StaticMesh.Get());
				if (Mesh.IsDynamic())
				{
					return true;
				}
			}
		}

		// Otherwise, scan and load pre-defined mesh data.

		if (const TConstStructView<FFaerieSkeletalMeshData> SkelMeshData = Token->GetSkeletalItemMesh(PurposeHierarchy);
			SkelMeshData.IsValid())
		{
			Mesh = FFaerieItemMesh::MakeSkeletal(SkelMeshData.Get());
			return true;
		}

		if (const TConstStructView<FFaerieStaticMeshData> StaticMeshData = Token->GetStaticItemMesh(PurposeHierarchy);
			StaticMeshData.IsValid())
		{
			Mesh = FFaerieItemMesh::MakeStatic(StaticMeshData.Get());
			return true;
		}

		UE_LOG(LogFaerieItemMesh, Error, TEXT("%hs: Asset does not contain a mesh suitable for the purpose."), __FUNCTION__)
		return false;
	}

	bool LoadMeshFromProxySynchronous(const FFaerieItemProxy Proxy, const FGameplayTag Purpose, FFaerieItemMesh& Mesh)
	{
		if (!ensure(Proxy.IsValid()))
		{
			UE_LOG(LogFaerieItemMesh, Warning, TEXT("%hs: Invalid proxy!"), __FUNCTION__)
			return false;
		}

		if (!IsValid(Proxy->GetItemObject()))
		{
			UE_LOG(LogFaerieItemMesh, Error, TEXT("%hs: Invalid item object!"), __FUNCTION__)
			return false;
		}

		if (auto&& MeshToken = Proxy->GetItemObject()->GetToken<UFaerieMeshTokenBase>())
		{
			return LoadMeshFromTokenSynchronous(MeshToken, Purpose, Mesh);
		}
		return false;
	}
}

FFaerieItemMesh UFaerieItemMeshLoader::GetDynamicStaticMeshForData(const FFaerieDynamicStaticMesh& MeshData)
{
	return Faerie::GetDynamicStaticMeshForData(MeshData);
}

FFaerieItemMesh UFaerieItemMeshLoader::GetDynamicSkeletalMeshForData(const FFaerieDynamicSkeletalMesh& MeshData)
{
	return Faerie::GetDynamicSkeletalMeshForData(MeshData);
}

bool UFaerieItemMeshLoader::LoadMeshFromTokenSynchronous(const UFaerieMeshTokenBase* Token, const FGameplayTag Purpose,
														 FFaerieItemMesh& Mesh)
{
	return Faerie::LoadMeshFromTokenSynchronous(Token, Purpose, Mesh);
}

bool UFaerieItemMeshLoader::LoadMeshFromProxySynchronous(const FFaerieItemProxy Proxy, const FGameplayTag Purpose,
														 FFaerieItemMesh& Mesh)
{
	return Faerie::LoadMeshFromProxySynchronous(Proxy, Purpose, Mesh);
}

void UFaerieItemMeshLoader::LoadMeshFromTokenAsynchronous(const UFaerieMeshTokenBase* Token, const FGameplayTag Purpose,
	Faerie::FItemMeshAsyncLoadResult Callback)
{
	if (!IsValid(Token))
	{
		UE_LOG(LogFaerieItemMesh, Warning, TEXT("%hs: No MeshToken on entry"), __FUNCTION__)
		(void)Callback.ExecuteIfBound(false, {});
	}

	Faerie::FAsyncLoadRequest LoadRequest;
	LoadRequest.Token = Token;
	LoadRequest.Purpose = Purpose;
	LoadRequest.Callback = MoveTemp(Callback);

	FGameplayTagContainer PurposeHierarchy;
	if (Purpose.IsValid() && Purpose != Faerie::ItemMesh::Tags::MeshPurpose_Default)
	{
		if (ensure(Purpose.GetTagName().IsValid()))
		{
			PurposeHierarchy.AddTagFast(Purpose);
		}
	}
	PurposeHierarchy.AddTagFast(Faerie::ItemMesh::Tags::MeshPurpose_Default);

	// Check for the presence of a custom dynamic mesh to build.

	if (auto&& DynamicMeshToken = Cast<UFaerieMeshToken_Dynamic>(Token))
	{
		if (auto&& SkeletalMesh = DynamicMeshToken->GetDynamicSkeletalItemMesh(PurposeHierarchy);
			SkeletalMesh.IsValid())
		{
			const FFaerieDynamicSkeletalMesh& MeshData = SkeletalMesh.Get();
			TArray<FSoftObjectPath> AssetsToLoad;

			for (auto&& Fragment : MeshData.Fragments)
			{
				if (Fragment.SkeletonAndAnimation.Mesh.IsPending())
				{
					AssetsToLoad.Add(Fragment.SkeletonAndAnimation.Mesh.ToSoftObjectPath());
				}
				if (Fragment.SkeletonAndAnimation.AnimClass.IsPending())
				{
					AssetsToLoad.Add(Fragment.SkeletonAndAnimation.AnimClass.ToSoftObjectPath());
				}
				for (auto&& Material : Fragment.Materials)
				{
					if (Material.Material.IsPending())
					{
						AssetsToLoad.Add(Material.Material.ToSoftObjectPath());
					}
				}
			}

			if (AssetsToLoad.IsEmpty())
			{
				OnAsyncDynamicSkeletalMeshLoaded(SkeletalMesh, MoveTemp(LoadRequest));
			}
			else
			{
				UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(AssetsToLoad),
					FStreamableDelegate::CreateUObject(this, &ThisClass::OnAsyncDynamicSkeletalMeshLoaded, SkeletalMesh, LoadRequest));
			}
			return;
		}

		if (auto&& StaticMesh = DynamicMeshToken->GetDynamicStaticItemMesh(PurposeHierarchy);
			StaticMesh.IsValid())
		{
			const FFaerieDynamicStaticMesh& MeshData = StaticMesh.Get();
			TArray<FSoftObjectPath> AssetsToLoad;

			for (auto&& Fragment : MeshData.Fragments)
			{
				if (Fragment.StaticMesh.IsPending())
				{
					AssetsToLoad.Add(Fragment.StaticMesh.ToSoftObjectPath());
				}
				for (auto&& Material : Fragment.Materials)
				{
					if (Material.Material.IsPending())
					{
						AssetsToLoad.Add(Material.Material.ToSoftObjectPath());
					}
				}
			}

			if (AssetsToLoad.IsEmpty())
			{
				OnAsyncDynamicStaticMeshLoaded(StaticMesh, MoveTemp(LoadRequest));
			}
			else
			{
				UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(AssetsToLoad),
					FStreamableDelegate::CreateUObject(this, &ThisClass::OnAsyncDynamicStaticMeshLoaded, StaticMesh, LoadRequest));
			}
			return;
		}
	}

	// Otherwise, scan and load pre-defined mesh data.

	if (const TConstStructView<FFaerieSkeletalMeshData> SkelMeshData = Token->GetSkeletalItemMesh(PurposeHierarchy);
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
			UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(AssetsToLoad),
				FStreamableDelegate::CreateUObject(this, &ThisClass::OnAsyncSkeletalMeshLoaded, SkelMeshData, LoadRequest));
		}
		return;
	}

	if (const TConstStructView<FFaerieStaticMeshData> StaticMeshData = Token->GetStaticItemMesh(PurposeHierarchy);
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
			UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(AssetsToLoad),
				FStreamableDelegate::CreateUObject(this, &ThisClass::OnAsyncStaticMeshLoaded, StaticMeshData, LoadRequest));
		}
		return;
	}

	UE_LOG(LogFaerieItemMesh, Error, TEXT("%hs: Asset does not contain a mesh suitable for the purpose."), __FUNCTION__)
	(void)Callback.ExecuteIfBound(false, {});
}

void UFaerieItemMeshLoader::LoadMeshFromProxyAsynchronous(const FFaerieItemProxy Proxy, const FGameplayTag Purpose,
														  Faerie::FItemMeshAsyncLoadResult Callback)
{
	if (!ensure(Proxy.IsValid()))
	{
		UE_LOG(LogFaerieItemMesh, Warning, TEXT("%hs: Invalid proxy!"), __FUNCTION__)
		(void)Callback.ExecuteIfBound(false, {});
	}

	if (!IsValid(Proxy->GetItemObject()))
	{
		UE_LOG(LogFaerieItemMesh, Error, TEXT("%hs: Invalid item object!"), __FUNCTION__)
		(void)Callback.ExecuteIfBound(false, {});
	}

	if (auto&& MeshToken = Proxy->GetItemObject()->GetToken<UFaerieMeshTokenBase>())
	{
		return LoadMeshFromTokenAsynchronous(MeshToken, Purpose, MoveTemp(Callback));
	}
}

void UFaerieItemMeshLoader::OnAsyncStaticMeshLoaded(const TConstStructView<FFaerieStaticMeshData> MeshData,
	Faerie::FAsyncLoadRequest Request)
{
	const FFaerieItemMesh Mesh = FFaerieItemMesh::MakeStatic(MeshData.Get());
	HandleAsyncLoadResult(Mesh, MoveTemp(Request));
}

void UFaerieItemMeshLoader::OnAsyncSkeletalMeshLoaded(const TConstStructView<FFaerieSkeletalMeshData> MeshData,
	Faerie::FAsyncLoadRequest Request)
{
	const FFaerieItemMesh Mesh = FFaerieItemMesh::MakeSkeletal(MeshData.Get());
	HandleAsyncLoadResult(Mesh, MoveTemp(Request));
}

void UFaerieItemMeshLoader::OnAsyncDynamicStaticMeshLoaded(const TConstStructView<FFaerieDynamicStaticMesh> MeshData,
	Faerie::FAsyncLoadRequest Request)
{
	const FFaerieItemMesh Mesh = GetDynamicStaticMeshForData(MeshData.Get());
	HandleAsyncLoadResult(Mesh, MoveTemp(Request));
}

void UFaerieItemMeshLoader::OnAsyncDynamicSkeletalMeshLoaded(const TConstStructView<FFaerieDynamicSkeletalMesh> MeshData,
	Faerie::FAsyncLoadRequest Request)
{
	const FFaerieItemMesh Mesh = GetDynamicSkeletalMeshForData(MeshData.Get());
	HandleAsyncLoadResult(Mesh, MoveTemp(Request));
}

void UFaerieItemMeshLoader::HandleAsyncLoadResult(const FFaerieItemMesh& Mesh, Faerie::FAsyncLoadRequest&& Request)
{
	(void)Request.Callback.ExecuteIfBound(true, Mesh);
}

bool UFaerieItemMeshLoader_Cached::LoadMeshFromTokenSynchronous(const UFaerieMeshTokenBase* Token, const FGameplayTag Purpose, FFaerieItemMesh& Mesh)
{
	const FFaerieCachedMeshKey Key = {Token, Purpose};

	// If we have already generated this mesh, return that one.
	if (auto&& CachedMesh = GeneratedMeshes.Find(Key))
	{
		Mesh = *CachedMesh;
		return true;
	}

	const bool SuperResult = Super::LoadMeshFromTokenSynchronous(Token, Purpose, Mesh);

	// If the mesh load succeeded, cache the result.
	if (SuperResult)
	{
		GeneratedMeshes.Add(Key, Mesh);
	}

	return SuperResult;
}

void UFaerieItemMeshLoader_Cached::HandleAsyncLoadResult(const FFaerieItemMesh& Mesh,
	Faerie::FAsyncLoadRequest&& Request)
{
	const FFaerieCachedMeshKey Key = {Request.Token, Request.Purpose};
	GeneratedMeshes.Add(Key, Mesh);
	Super::HandleAsyncLoadResult(Mesh, MoveTemp(Request));
}

void UFaerieItemMeshLoader_Cached::ResetCache()
{
	GeneratedMeshes.Reset();
}

void UFaerieItemMeshLoader_Cached::ResetCacheByKey(const UFaerieMeshTokenBase* Token, const FGameplayTag Purpose)
{
	const FFaerieCachedMeshKey Key = {Token, Purpose};
	GeneratedMeshes.Remove(Key);
}
