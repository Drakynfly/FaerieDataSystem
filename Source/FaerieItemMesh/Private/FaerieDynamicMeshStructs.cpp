// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieDynamicMeshStructs.h"
#include "FaerieItemMeshLog.h"

FFaerieDynamicStaticMesh::FFaerieDynamicStaticMesh(const FFaerieStaticMeshData& MeshData)
{
	if (!MeshData.StaticMesh.IsValid())
	{
		UE_LOG(LogFaerieItemMesh, Warning, TEXT("FFaerieDynamicStaticMesh constructed from invalid MeshData"))
		return;
	}

	FFaerieDynamicStaticMeshFragment SingleFragment;
	SingleFragment.StaticMesh = MeshData.StaticMesh;
	SingleFragment.Materials = MeshData.Materials;
	Fragments.Add(SingleFragment);
	Purpose = MeshData.Purpose;
}

FFaerieDynamicSkeletalMesh::FFaerieDynamicSkeletalMesh(const FFaerieSkeletalMeshData& MeshData)
{
	if (!MeshData.SkeletonAndAnimation.Mesh.IsValid())
	{
		UE_LOG(LogFaerieItemMesh, Warning, TEXT("FFaerieDynamicSkeletalMesh constructed from invalid MeshData"))
		return;
	}

	FFaerieDynamicSkeletalMeshFragment SingleFragment;
	SingleFragment.SkeletonAndAnimation = MeshData.SkeletonAndAnimation;
	SingleFragment.Materials = MeshData.Materials;
	Fragments.Add(SingleFragment);
	Purpose = MeshData.Purpose;
}

FFaerieDynamicMeshContainer::FFaerieDynamicMeshContainer(const FFaerieMeshContainer& MeshContainer)
{
	for (const FFaerieStaticMeshData& EditorStaticMesh : MeshContainer.StaticMeshes)
	{
		StaticMeshes.Add(EditorStaticMesh);
	}
	for (const FFaerieSkeletalMeshData& EditorSkeletalMesh : MeshContainer.SkeletalMeshes)
	{
		SkeletalMeshes.Add(EditorSkeletalMesh);
	}
}

TConstStructView<FFaerieDynamicStaticMesh> FFaerieDynamicMeshContainer::GetStaticItemMesh(
	const FGameplayTagContainer& SearchPurposes) const
{
	for (const FFaerieDynamicStaticMesh& Data : StaticMeshes)
	{
		if (Data.Purpose.HasAnyExact(SearchPurposes))
		{
			return Data;
		}
	}
	return TConstStructView<FFaerieDynamicStaticMesh>();
}

TConstStructView<FFaerieDynamicSkeletalMesh> FFaerieDynamicMeshContainer::GetSkeletalItemMesh(
	const FGameplayTagContainer& SearchPurposes) const
{
	for (const FFaerieDynamicSkeletalMesh& Data : SkeletalMeshes)
	{
		if (Data.Purpose.HasAnyExact(SearchPurposes))
		{
			return Data;
		}
	}
	return TConstStructView<FFaerieDynamicSkeletalMesh>();
}
