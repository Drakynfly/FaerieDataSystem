// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMeshStructs.h"
#include "UDynamicMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMeshStructs)

namespace Faerie::ItemMesh::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeshPurpose_Default, FName{TEXTVIEW("MeshPurpose.Default")}, "Only mesh, or mesh used as fallback if others fail")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeshPurpose_Display, FName{TEXTVIEW("MeshPurpose.Display")}, "Mesh for visual display, e.g item pickups.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeshPurpose_Equipped, FName{TEXTVIEW("MeshPurpose.Equipped")}, "Mesh for item when used as active equipment")
}

TConstStructView<FFaerieStaticMeshData> FFaerieMeshContainer::GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	for (const FFaerieStaticMeshData& Data : StaticMeshes)
	{
		if (Data.Purpose.HasAnyExact(SearchPurposes))
		{
			return Data;
		}
	}
	return TConstStructView<FFaerieStaticMeshData>();
}

TConstStructView<FFaerieSkeletalMeshData> FFaerieMeshContainer::GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	for (const FFaerieSkeletalMeshData& Data : SkeletalMeshes)
	{
		if (Data.Purpose.HasAnyExact(SearchPurposes))
		{
			return Data;
		}
	}
	return TConstStructView<FFaerieSkeletalMeshData>();
}

FFaerieItemMesh FFaerieItemMesh::MakeStatic(const FFaerieStaticMeshData& MeshData)
{
	FFaerieItemMesh ItemMesh;
	ItemMesh.StaticMesh = MeshData.StaticMesh.LoadSynchronous();
	ItemMesh.Materials.Reserve(MeshData.Materials.Num());
	for (auto&& Element : MeshData.Materials)
	{
		ItemMesh.Materials.Emplace(Element.Material.LoadSynchronous());
	}
	return ItemMesh;
}

FFaerieItemMesh FFaerieItemMesh::MakeStatic(UStaticMesh* Mesh, const TArray<FFaerieItemMaterial>& Materials)
{
	FFaerieItemMesh ItemMesh;
	ItemMesh.StaticMesh = Mesh;
	ItemMesh.Materials = Materials;
	return ItemMesh;
}

FFaerieItemMesh FFaerieItemMesh::MakeDynamic(UDynamicMesh* Mesh, const TArray<FFaerieItemMaterial>& Materials)
{
	FFaerieItemMesh ItemMesh;
	ItemMesh.DynamicStaticMesh = Mesh;
	ItemMesh.Materials = Materials;
	return ItemMesh;
}

FFaerieItemMesh FFaerieItemMesh::MakeSkeletal(const FFaerieSkeletalMeshData& MeshData)
{
	FFaerieItemMesh ItemMesh;
	ItemMesh.SkeletonAndAnimation = MeshData.SkeletonAndAnimation.LoadSynchronous();
	ItemMesh.Materials.Reserve(MeshData.Materials.Num());
	for (auto&& Element : MeshData.Materials)
	{
		ItemMesh.Materials.Emplace(Element.Material.LoadSynchronous());
	}
	return ItemMesh;
}

FFaerieItemMesh FFaerieItemMesh::MakeSkeletal(const FSkeletonAndAnimation& Mesh,
	const TArray<FFaerieItemMaterial>& Materials)
{
	FFaerieItemMesh ItemMesh;
	ItemMesh.SkeletonAndAnimation = Mesh;
	ItemMesh.Materials = Materials;
	return ItemMesh;
}

bool FFaerieItemMesh::IsStatic() const
{
	return IsValid(StaticMesh);
}

bool FFaerieItemMesh::IsDynamic() const
{
	return IsValid(DynamicStaticMesh);
}

bool FFaerieItemMesh::IsSkeletal() const
{
	return IsValid(SkeletonAndAnimation.Mesh);
}