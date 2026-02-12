// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "AnimUtilityStructs.h"
#include "NativeGameplayTags.h"
#include "StructUtils/StructView.h"
#include "FaerieMeshStructs.generated.h"

class UDynamicMesh;

namespace Faerie::ItemMesh::Tags
{
	FAERIEITEMMESH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeshPurpose_Default)
	FAERIEITEMMESH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeshPurpose_Display)
	FAERIEITEMMESH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeshPurpose_Equipped)
}

/**
 * Material reference for a faerie item
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieItemSoftMaterial
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieItemSoftMaterial")
	TSoftObjectPtr<UMaterialInterface> Material = nullptr;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemSoftMaterial& Other) const
	{
		return Material == Other.Material;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemSoftMaterial& FaerieItemSoftMaterial)
	{
		return GetTypeHash(FaerieItemSoftMaterial.Material);
	}
};

/**
 * Material reference for a faerie item
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieItemMaterial
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieItemMaterial")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemMaterial& Other) const
	{
		return Material == Other.Material;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemMaterial& FaerieItemMaterial)
	{
		return GetTypeHash(FaerieItemMaterial.Material);
	}
};

/**
 * Static mesh source data.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieStaticMeshData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieStaticMesh")
	TSoftObjectPtr<UStaticMesh> StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FFaerieItemSoftMaterial> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MeshPurpose"), Category = "FaerieMesh")
	FGameplayTagContainer Purpose;
};

/**
 * Skeletal mesh source data.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieSkeletalMeshData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkeletalMeshData")
	FSoftSkeletonAndAnimation SkeletonAndAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FFaerieItemSoftMaterial> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MeshPurpose"), Category = "SkeletalMeshData")
	FGameplayTagContainer Purpose;
};


/**
 * A mesh data container for editor defined static and skeletal meshes.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieMeshContainer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MeshContainer", meta = (Categories = "MeshPurpose"))
	TArray<FFaerieStaticMeshData> StaticMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MeshContainer", meta = (Categories = "MeshPurpose"))
	TArray<FFaerieSkeletalMeshData> SkeletalMeshes;

	TConstStructView<FFaerieStaticMeshData> GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const;
	TConstStructView<FFaerieSkeletalMeshData> GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const;
};

/**
 * An item mesh container. Carries either a static, skeletal, or dynamic mesh, and any materials for them.
 * Usually only one of the three mesh options will be valid.
 * This struct contains loaded meshes and materials.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieItemMesh
{
	GENERATED_BODY()

	// Warning: This causes a Synchronous Load if the mesh is not preloaded.
	static FFaerieItemMesh MakeStatic(const FFaerieStaticMeshData& MeshData);

	static FFaerieItemMesh MakeStatic(UStaticMesh* Mesh, const TArray<FFaerieItemMaterial>& Materials);

	static FFaerieItemMesh MakeDynamic(UDynamicMesh* Mesh, const TArray<FFaerieItemMaterial>& Materials);

	// Warning: This causes a Synchronous Load if the mesh is not preloaded.
	static FFaerieItemMesh MakeSkeletal(const FFaerieSkeletalMeshData& MeshData);

	static FFaerieItemMesh MakeSkeletal(const FSkeletonAndAnimation& Mesh, const TArray<FFaerieItemMaterial>& Materials);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieItemMesh")
	TObjectPtr<const UStaticMesh> StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieItemMesh")
	TObjectPtr<UDynamicMesh> DynamicStaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieItemMesh")
	FSkeletonAndAnimation SkeletonAndAnimation;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FaerieItemMesh")
	TArray<FFaerieItemMaterial> Materials;

	bool IsStatic() const;
	bool IsDynamic() const;
	bool IsSkeletal() const;

	const UStaticMesh* GetStatic() const
	{
		return StaticMesh;
	}

	// Non-const version while UE doesn't treat these const-safely.
	UStaticMesh* GetStatic_Unsafe() const
	{
		return ConstCast(StaticMesh);
	}

	UDynamicMesh* GetDynamic() const
	{
		return DynamicStaticMesh;
	}

	FSkeletonAndAnimation GetSkeletal() const
	{
		return SkeletonAndAnimation;
	}

	void SetStatic(const UStaticMesh* Mesh)
	{
		StaticMesh = Mesh;
	}

	void SetDynamic(UDynamicMesh* Mesh)
	{
		DynamicStaticMesh = Mesh;
	}

	void SetSkeletal(const FSkeletonAndAnimation& Mesh)
	{
		SkeletonAndAnimation = Mesh;
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemMesh& Other) const
	{
		return StaticMesh == Other.StaticMesh
			&& DynamicStaticMesh == Other.DynamicStaticMesh
			&& SkeletonAndAnimation == Other.SkeletonAndAnimation
			&& Materials == Other.Materials;
	}
};