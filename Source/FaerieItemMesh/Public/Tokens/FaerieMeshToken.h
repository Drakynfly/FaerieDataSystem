// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieMeshStructs.h"

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
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Inventory Asset|Item Info", meta = (AutoCreateRefTerm = "SearchPurposes"))
	virtual bool GetStaticItemMesh(UPARAM(meta = (Categories = "MeshPurpose")) const FGameplayTagContainer& SearchPurposes,
		FFaerieStaticMeshData& Static) const { return false; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Inventory Asset|Item Info", meta = (AutoCreateRefTerm = "SearchPurposes"))
	virtual bool GetSkeletalItemMesh(UPARAM(meta = (Categories = "MeshPurpose")) const FGameplayTagContainer& SearchPurposes,
		FFaerieSkeletalMeshData& Skeletal) const { return false; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Inventory Asset|Item Info", meta = (AutoCreateRefTerm = "SearchPurposes"))
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

	virtual bool GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes, FFaerieStaticMeshData& Static) const override;
	virtual bool GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes, FFaerieSkeletalMeshData& Skeletal) const override;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MeshToken", meta = (ShowOnlyInnerProperties, ExposeOnSpawn))
	FFaerieDynamicMeshContainer DynamicMeshContainer;
};