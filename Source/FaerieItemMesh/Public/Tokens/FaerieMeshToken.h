// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMeshStructs.h"
#include "FaerieDynamicMeshStructs.h"
#include "FaerieItemToken.h"

#include "FaerieMeshToken.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMeshToken, Log, All);

/**
 *
 */
UCLASS(Abstract)
class FAERIEITEMMESH_API UFaerieMeshTokenBase : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	virtual TConstStructView<FFaerieStaticMeshData> GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const
		PURE_VIRTUAL(UFaerieMeshTokenBase::GetStaticItemMesh, return TConstStructView<FFaerieStaticMeshData>(); )
	virtual TConstStructView<FFaerieSkeletalMeshData> GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const
		PURE_VIRTUAL(UFaerieMeshTokenBase::GetSkeletalItemMesh, return TConstStructView<FFaerieSkeletalMeshData>(); )

	void GetMeshes(const FGameplayTagContainer& SearchPurposes,
		TConstStructView<FFaerieStaticMeshData>& Static,
		TConstStructView<FFaerieSkeletalMeshData>& Skeletal) const;

protected:
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|MeshToken", meta = (AutoCreateRefTerm = "SearchPurposes"))
	bool GetStaticItemMesh(UPARAM(meta = (Categories = "MeshPurpose")) const FGameplayTagContainer& SearchPurposes,
		FFaerieStaticMeshData& Static) const { return false; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|MeshToken", meta = (AutoCreateRefTerm = "SearchPurposes"))
	bool GetSkeletalItemMesh(UPARAM(meta = (Categories = "MeshPurpose")) const FGameplayTagContainer& SearchPurposes,
		FFaerieSkeletalMeshData& Skeletal) const { return false; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|MeshToken", meta = (AutoCreateRefTerm = "SearchPurposes"))
	void GetMeshes(UPARAM(meta = (Categories = "MeshPurpose")) const FGameplayTagContainer& SearchPurposes, bool& FoundStatic,
		FFaerieStaticMeshData& Static, bool& FoundSkeletal, FFaerieSkeletalMeshData& Skeletal) const;
};


/**
 *
 */
UCLASS(DisplayName = "Token - Visual: Mesh")
class FAERIEITEMMESH_API UFaerieMeshToken : public UFaerieMeshTokenBase
{
	GENERATED_BODY()

	friend class UFaerieMeshSubsystem;

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual TConstStructView<FFaerieStaticMeshData> GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const override;
	virtual TConstStructView<FFaerieSkeletalMeshData> GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const override;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "MeshToken", meta = (ShowOnlyInnerProperties, ExposeOnSpawn))
	FFaerieMeshContainer MeshContainer;
};

// Dynamic mesh tokens are generated via crafting, they shouldn't be added to items manually, hence the HideDropdown
UCLASS(HideDropdown, DisplayName = "Token - Mesh (Dynamic)")
class UFaerieMeshToken_Dynamic : public UFaerieMeshTokenBase
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual TConstStructView<FFaerieStaticMeshData> GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const override;
	virtual TConstStructView<FFaerieSkeletalMeshData> GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const override;

	TConstStructView<FFaerieDynamicStaticMesh> GetDynamicStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const;
	TConstStructView<FFaerieDynamicSkeletalMesh> GetDynamicSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MeshToken", meta = (ShowOnlyInnerProperties, ExposeOnSpawn))
	FFaerieDynamicMeshContainer DynamicMeshContainer;
};