// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"

#include "FaerieMeshStructs.h"
#include "ItemMeshType.h"

#include "FaerieItemMeshComponent.generated.h"

namespace Faerie::Ed
{
	struct FItemPreviewSceneData;
}

class UFaerieItemMeshComponent;
class UFaerieMeshTokenBase;
class UMeshComponent;

namespace Faerie
{
	using FOnMeshRebuiltEvent = TMulticastDelegate<void(UFaerieItemMeshComponent*)>;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFaerieOnMeshRebuilt);

UCLASS(ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent))
class FAERIEITEMMESH_API UFaerieItemMeshComponent : public USceneComponent
{
	GENERATED_BODY()

	friend Faerie::Ed::FItemPreviewSceneData;

public:
	UFaerieItemMeshComponent();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void DestroyComponent(bool bPromoteChildren = false) override;

protected:
	void UpdateCachedBounds();

	void LoadMeshFromToken(bool Async);
	void AsyncLoadMeshReturn(bool Success, FFaerieItemMesh&& InMeshData);

	void RebuildMesh();

	UFUNCTION(/* Replication */)
	void OnRep_SourceMeshToken();

	UFUNCTION(/* Replication */)
	void OnRep_SkeletalMeshLeader();

	UFUNCTION(/* Replication */)
	void OnRep_PreferredTag();

public:
	Faerie::FOnMeshRebuiltEvent::RegistrationType& GetOnMeshRebuilt() { return OnMeshRebuiltNative; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataMesh")
	UMeshComponent* GetGeneratedMeshComponent() const { return MeshComponent; }

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
	Faerie::FOnMeshRebuiltEvent OnMeshRebuiltNative;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FFaerieOnMeshRebuilt OnMeshRebuilt;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = "OnRep_SourceMeshToken", BlueprintReadOnly, Category = "Config")
	TObjectPtr<const UFaerieMeshTokenBase> SourceMeshToken;

	// Leader component if LeaderPose is allowed.
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = "OnRep_SkeletalMeshLeader", BlueprintReadOnly, Category = "Config")
	TObjectPtr<USkinnedMeshComponent> SkeletalMeshLeader;

	// The MeshPurpose preferred by this Component. Only matters if SourceMeshToken is set.
	UPROPERTY(EditAnywhere, ReplicatedUsing = "OnRep_PreferredTag", BlueprintReadOnly, Category = "Config", meta = (Categories = "MeshPurpose"))
	FGameplayTag PreferredTag;

	// If MeshData contains this type, it will be used.
	// Otherwise, they will be chosen in the order static->dynamic->skeletal.
	// @todo this doesn't work, as MeshData almost always only contains a single value unless SetItemMesh is manually called.
	// @todo if this is fixed, then this should replicate as well.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	EItemMeshType PreferredType;

	// Should a warning be logged when rebuilt with an invalid mesh.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool WarnIfMeshInvalid = false;

	// Center the mesh according to its bounding box, instead of using its origin point.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool CenterMeshByBounds = false;

	// Calculate and cache the bounds of a posed skeletal mesh component.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool CacheSkeletalBoundsInPose = false;

	// If the mesh data does not have the preferred type, this stores the actual type used.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EItemMeshType ActualType;

	// Mesh data source.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FFaerieItemMesh MeshData;

	// Component generated at runtime to display the appropriate mesh from MeshData.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<UMeshComponent> MeshComponent;

	// The last cached bounds. Used for SkeletalMeshes when CacheSkeletalBoundsInPose is true.
	TOptional<FBoxSphereBounds> CachedBounds;
};