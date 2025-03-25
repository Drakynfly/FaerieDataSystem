// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMeshStructs.h"
#include "FaerieDynamicMeshStructs.generated.h"

/*
 * A composable piece of a dynamic static mesh.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieDynamicStaticMeshFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieStaticMesh")
	TSoftObjectPtr<UStaticMesh> StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FFaerieItemSoftMaterial> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieStaticMeshFragment")
	FSocketAttachment Attachment;
};

/*
 * A composable piece of a dynamic skeletal mesh.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieDynamicSkeletalMeshFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieSkeletalMesh")
	FSoftSkeletonAndAnimClass SkeletonAndAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FFaerieItemSoftMaterial> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieSkeletalMeshFragment")
	FSocketAttachment Attachment;
};

/*
 * A static mesh composed of multiple pieces.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieDynamicStaticMesh
{
	GENERATED_BODY()

	FFaerieDynamicStaticMesh() = default;

	FFaerieDynamicStaticMesh(const FFaerieStaticMeshData& MeshData);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieDynamicStaticMesh")
	TArray<FFaerieDynamicStaticMeshFragment> Fragments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MeshPurpose"), Category = "FaerieMesh")
	FGameplayTagContainer Purpose;

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieDynamicStaticMesh& FaerieDynamicStaticMesh)
	{
		return FCrc::MemCrc32(&FaerieDynamicStaticMesh, sizeof(FFaerieDynamicStaticMesh));
	}
};

/*
 * A skeletal mesh composed of multiple pieces.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieDynamicSkeletalMesh
{
	GENERATED_BODY()

	FFaerieDynamicSkeletalMesh() = default;

	FFaerieDynamicSkeletalMesh(const FFaerieSkeletalMeshData& MeshData);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FFaerieDynamicSkeletalMeshFragment> Fragments;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MeshPurpose"), Category = "FaerieMesh")
	FGameplayTagContainer Purpose;

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieDynamicSkeletalMesh& FaerieDynamicSkeletalMesh)
	{
		return FCrc::MemCrc32(&FaerieDynamicSkeletalMesh, sizeof(FFaerieDynamicSkeletalMesh));
	}
};

/**
 * A mesh data container for programatically defined static and skeletal meshes. These are created via crafting and can
 * contain "composable meshes".
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieDynamicMeshContainer
{
	GENERATED_BODY()

	FFaerieDynamicMeshContainer() = default;

	// Create a FFaerieDynamicMeshContainer to use in crafting from an asset's FFaerieMeshContainer.
	// All meshes will contain a single fragment, since regular containers only store asset meshes.
	FFaerieDynamicMeshContainer(const FFaerieMeshContainer& MeshContainer);

	UPROPERTY(BlueprintReadOnly, Category = "Meshes", meta = (Categories = "MeshPurpose"))
	TArray<FFaerieDynamicStaticMesh> StaticMeshes;

	UPROPERTY(BlueprintReadOnly, Category = "Meshes", meta = (Categories = "MeshPurpose"))
	TArray<FFaerieDynamicSkeletalMesh> SkeletalMeshes;

	TConstStructView<FFaerieDynamicStaticMesh> GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const;
	TConstStructView<FFaerieDynamicSkeletalMesh> GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const;
};
