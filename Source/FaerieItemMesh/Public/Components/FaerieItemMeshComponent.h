// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"

#include "FaerieMeshStructs.h"
#include "ItemMeshType.h"

#include "FaerieItemMeshComponent.generated.h"

class UFaerieMeshTokenBase;

UCLASS(ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent))
class FAERIEITEMMESH_API UFaerieItemMeshComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFaerieItemMeshComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void DestroyComponent(bool bPromoteChildren = false) override;

protected:
	void LoadMeshFromToken(bool Async);
	void AsyncLoadMeshReturn(bool Success, const FFaerieItemMesh& InMeshData);

	void RebuildMesh();

	UFUNCTION(/* Replication */)
	void OnRep_SourceMeshToken();

public:
	// Set the mesh to use directly. This is local only, as the ItemMesh struct does not replicate.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	void SetItemMesh(const FFaerieItemMesh& InMeshData);

	// Set the token to generate a mesh from. If called on the server, the client can generate the same mesh.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	void SetItemMeshFromToken(const UFaerieMeshTokenBase* InMeshToken);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	void SetSkeletalMeshLeaderPoseComponent(USkinnedMeshComponent* LeaderComponent);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	void ClearItemMesh();

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	void SetPreferredTag(UPARAM(meta = (Categories = "MeshPurpose")) FGameplayTag MeshTag);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	void SetPreferredMeshType(EItemMeshType MeshType);

	// Get the bound for the current mesh data type.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	FBoxSphereBounds GetBounds() const;

protected:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = "OnRep_SourceMeshToken", BlueprintReadOnly, Category = "Config")
	TObjectPtr<const UFaerieMeshTokenBase> SourceMeshToken;

	// Leader component if LeaderPose is allowed.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<USkinnedMeshComponent> SkeletalMeshLeader;

	// The MeshPurpose preferred by this Component. Only matters if SourceMeshToken is set.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (Categories = "MeshPurpose"))
	FGameplayTag PreferredTag;

	// If MeshData contains this type, it will be used.
	// Otherwise, they will be chosen in the order static->dynamic->skeletal.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	EItemMeshType PreferredType;

	// If this component is allowed to exist without any mesh then enable this.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool AllowNullMeshes = false;

	// If the mesh data does not have the preferred type, this stores the actual type used.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EItemMeshType ActualType;

	// Mesh data source.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FFaerieItemMesh MeshData;

	// Component generated at runtime to display the appropriate mesh from MeshData.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<UMeshComponent> MeshComponent;
};