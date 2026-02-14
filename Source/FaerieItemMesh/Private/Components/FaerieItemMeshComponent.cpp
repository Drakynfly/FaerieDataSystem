// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Components/FaerieItemMeshComponent.h"
#include "FaerieItemMeshLoader.h"
#include "FaerieItemMeshLog.h"
#include "FaerieMeshStructs.h"
#include "FaerieMeshSubsystem.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ConversionUtils/SceneComponentToDynamicMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "Libraries/FaerieMeshStructsLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Tokens/FaerieMeshToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemMeshComponent)

UFaerieItemMeshComponent::UFaerieItemMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PreferredTag = Faerie::ItemMesh::Tags::MeshPurpose_Display;
	PreferredType = EItemMeshType::Static;
	ActualType = EItemMeshType::None;
}

void UFaerieItemMeshComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SourceMeshToken, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SkeletalMeshLeader, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PreferredTag, SharedParams)
}

void UFaerieItemMeshComponent::DestroyComponent(const bool bPromoteChildren)
{
	if (IsValid(MeshComponent))
	{
		MeshComponent->DestroyComponent();
		MeshComponent = nullptr;
	}

	Super::DestroyComponent(bPromoteChildren);
}

void UFaerieItemMeshComponent::UpdateCachedBounds()
{
	static constexpr UE::Conversion::FToMeshOptions StaticToMeshOptions = []()
	{
		UE::Conversion::FToMeshOptions ToMeshOptions;
		ToMeshOptions.LODType = UE::Conversion::EMeshLODType::RenderData;
		ToMeshOptions.bUseClosestLOD = false;
		return ToMeshOptions;
	}();

	constexpr bool bTransformToWorld = false;
	static FTransform LocalToWorld;
	static FText ErrorMessage;

	QUICK_SCOPE_CYCLE_COUNTER(STAT_UBoundsCachingSkeletalMeshComponent_UpdateCachedBounds)

	// Make sure skeletal mesh has its animation initialized.
	Cast<USkeletalMeshComponent>(MeshComponent)->RefreshBoneTransforms();

	UE::Geometry::FDynamicMesh3 NewMesh;
	if (UE::Conversion::SceneComponentToDynamicMesh(MeshComponent, StaticToMeshOptions, bTransformToWorld, NewMesh, LocalToWorld, ErrorMessage))
	{
		CachedBounds = static_cast<FBox>(NewMesh.GetBounds(true));
	}
	else
	{
		CachedBounds.Reset();
	}
}

void UFaerieItemMeshComponent::LoadMeshFromToken(const bool Async)
{
	if (!IsValid(SourceMeshToken))
	{
		return;
	}

	if (AsyncMeshLoadingHandle.IsValid())
	{
		AsyncMeshLoadingHandle->CancelHandle();
		AsyncMeshLoadingHandle.Reset();
	}

	UFaerieMeshSubsystem* MeshSubsystem = GetWorld()->GetSubsystem<UFaerieMeshSubsystem>();
	if (!IsValid(MeshSubsystem))
	{
#if WITH_EDITOR
		// @todo: this is a hack to allow the editor to load meshes when world subsystems aren't available...
		auto Loader = GetMutableDefault<UFaerieItemMeshLoader>();
		if (Async)
		{
			AsyncMeshLoadingHandle = Loader->LoadMeshFromTokenAsynchronous(SourceMeshToken, PreferredTag,
				Faerie::FItemMeshAsyncLoadResult::CreateUObject(this, &ThisClass::AsyncLoadMeshReturn));
		}
		else
		{
			Loader->LoadMeshFromTokenSynchronous(SourceMeshToken, PreferredTag, MeshData);
			RebuildMesh();
		}
#endif
		return;
	}

	if (Async)
	{
		AsyncMeshLoadingHandle = MeshSubsystem->LoadMeshFromTokenAsynchronous(SourceMeshToken, PreferredTag,
			Faerie::FItemMeshAsyncLoadResult::CreateUObject(this, &ThisClass::AsyncLoadMeshReturn));
	}
	else
	{
		MeshSubsystem->LoadMeshFromTokenSynchronous(SourceMeshToken, PreferredTag, MeshData);
		RebuildMesh();
	}
}

void UFaerieItemMeshComponent::AsyncLoadMeshReturn(const bool Success, FFaerieItemMesh&& InMeshData)
{
	if (Success)
	{
		MeshData = MoveTemp(InMeshData);
		RebuildMesh();
	}
	else
	{
		ClearItemMesh();
	}

	if (AsyncMeshLoadingHandle.IsValid())
	{
		AsyncMeshLoadingHandle->CancelHandle();
		AsyncMeshLoadingHandle.Reset();
	}
}

void UFaerieItemMeshComponent::RebuildMesh()
{
	EItemMeshType NewMeshType = EItemMeshType::None;

	// Select new type by preferred first
	switch (PreferredType)
	{
	case EItemMeshType::None: break;
	case EItemMeshType::Static:
		if (MeshData.IsStatic())
		{
			NewMeshType = EItemMeshType::Static;
		}
		break;
	case EItemMeshType::Dynamic:
		if (MeshData.IsDynamic())
		{
			NewMeshType = EItemMeshType::Dynamic;
		}
		break;
	case EItemMeshType::Skeletal:
		if (MeshData.IsSkeletal())
		{
			NewMeshType = EItemMeshType::Skeletal;
		}
		break;
	default: checkNoEntry();
	}

	// If the new type is not valid yet, select by what is actually present instead.
	if (NewMeshType == EItemMeshType::None)
	{
		if (MeshData.IsStatic())
		{
			NewMeshType = EItemMeshType::Static;
		}
		else if (MeshData.IsDynamic())
		{
			NewMeshType = EItemMeshType::Dynamic;
		}
		else if (MeshData.IsSkeletal())
		{
			NewMeshType = EItemMeshType::Skeletal;
		}
	}

	// If we are switching types, then destroy the old component if it exists.
	if (NewMeshType != ActualType)
	{
		if (IsValid(MeshComponent))
		{
			MeshComponent->DestroyComponent();
			MeshComponent = nullptr;
		}

		ActualType = NewMeshType;
	}

	// Warn if we have switched to None illegally.
	if (NewMeshType == EItemMeshType::None && WarnIfMeshInvalid)
	{
		UE_LOG(LogFaerieItemMesh, Error, TEXT("No valid mesh in Mesh Data."))
		return;
	}

	// Create the appropriate new mesh component for the type.
	if (!IsValid(MeshComponent))
	{
#if WITH_EDITOR
		// If we are in a runtime context where we have an owning actor, use that.
		UObject* Outer;
		if (GetOwner())
		{
			Outer = GetOwner();
		}
		// But in editor-only contexts, just parent the mesh components to the world.
		else
		{
			Outer = GetWorld();
		}
#else
		UObject* Outer = GetOwner();
#endif

		switch (ActualType)
		{
		case EItemMeshType::None: break;
		case EItemMeshType::Static:
			MeshComponent = NewObject<UStaticMeshComponent>(Outer);
			break;
		case EItemMeshType::Dynamic:
			MeshComponent = NewObject<UDynamicMeshComponent>(Outer);
			break;
		case EItemMeshType::Skeletal:
			MeshComponent = NewObject<USkeletalMeshComponent>(Outer);
			break;
		default: checkNoEntry();
		}

		MeshComponent->RegisterComponentWithWorld(GetWorld());

		// *This* component controls the transform, so hard snap the mesh component to ourself.
		MeshComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
	}

	bool CanCenterByBounds = CenterMeshByBounds;

	// Load the mesh and materials to the mesh component.
	switch (ActualType)
	{
	case EItemMeshType::None: break;
	case EItemMeshType::Static:
		if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(MeshComponent))
		{
			StaticMesh->SetStaticMesh(MeshData.GetStatic_Unsafe());
			for (int32 i = 0; i < MeshData.Materials.Num(); ++i)
			{
				StaticMesh->SetMaterial(i, MeshData.Materials[i].Material);
			}
		}
		break;
	case EItemMeshType::Dynamic:
		if (UDynamicMeshComponent* DynamicMesh = Cast<UDynamicMeshComponent>(MeshComponent))
		{
			DynamicMesh->SetDynamicMesh(MeshData.GetDynamic());
			DynamicMesh->ConfigureMaterialSet(UFaerieMeshStructsLibrary::FaerieItemMaterialsToObjectArray(MeshData.Materials));
		}
		break;
	case EItemMeshType::Skeletal:
		if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			auto&& Skeletal = MeshData.GetSkeletal();
			SkeletalMesh->SetSkeletalMesh(ConstCast(Skeletal.Mesh));

			// Try to set up animation. Disable CenterByBounds while animating.
			if (IsValid(Skeletal.AnimClass))
			{
				SkeletalMesh->SetAnimInstanceClass(Skeletal.AnimClass);
				//CanCenterByBounds = false;
			}
			else if (IsValid(Skeletal.AnimationAsset))
			{
				SkeletalMesh->PlayAnimation(Skeletal.AnimationAsset, true);
				//CanCenterByBounds = false;
			}
			else if (IsValid(SkeletalMeshLeader))
			{
				// If there is no Animation Class or Asset, maybe we were supposed to be a LeaderPose component.
				SkeletalMesh->SetLeaderPoseComponent(SkeletalMeshLeader, true);
				CanCenterByBounds = false;
			}

			// Setup Materials
			for (int32 i = 0; i < MeshData.Materials.Num(); ++i)
			{
				SkeletalMesh->SetMaterial(i, MeshData.Materials[i].Material);
			}

			if (CacheSkeletalBoundsInPose)
			{
				UpdateCachedBounds();
			}
		}
		break;
	default: checkNoEntry();
	}

	if (CanCenterByBounds)
	{
		const FBoxSphereBounds MeshBounds = GetBounds();
		MeshComponent->AddLocalOffset(-MeshBounds.Origin);
	}

#if WITH_EDITOR
	MeshComponent->bSelectable = true;
#endif

	OnMeshRebuiltNative.Broadcast(this);
	OnMeshRebuilt.Broadcast();
}

void UFaerieItemMeshComponent::OnRep_SourceMeshToken()
{
	LoadMeshFromToken(true);
}

void UFaerieItemMeshComponent::OnRep_SkeletalMeshLeader()
{
	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(MeshComponent))
	{
		if (IsValid(SkeletalMeshLeader))
		{
			SkeletalMesh->SetLeaderPoseComponent(SkeletalMeshLeader, true);
		}
		else
		{
			SkeletalMesh->SetLeaderPoseComponent(nullptr, true);
		}
	}
}

void UFaerieItemMeshComponent::OnRep_PreferredTag()
{
	RebuildMesh();
	LoadMeshFromToken(true);
}

void UFaerieItemMeshComponent::SetItemMesh(const FFaerieItemMesh& InMeshData)
{
	if (MeshData != InMeshData)
	{
		COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, SourceMeshToken, nullptr, this);

		MeshData = InMeshData;
		RebuildMesh();
	}
}

void UFaerieItemMeshComponent::SetItemMeshFromToken(const UFaerieMeshTokenBase* InMeshToken)
{
	if (SourceMeshToken != InMeshToken)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SourceMeshToken, this);
		SourceMeshToken = InMeshToken;
		LoadMeshFromToken(true);
	}
}

void UFaerieItemMeshComponent::SetSkeletalMeshLeaderPoseComponent(USkinnedMeshComponent* LeaderComponent)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, SkeletalMeshLeader, LeaderComponent, this);
}

void UFaerieItemMeshComponent::ClearItemMesh()
{
	if (AsyncMeshLoadingHandle.IsValid())
	{
		AsyncMeshLoadingHandle->CancelHandle();
		AsyncMeshLoadingHandle.Reset();
	}

	ActualType = EItemMeshType::None;
	MeshData = FFaerieItemMesh();

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SourceMeshToken, this);
	SourceMeshToken = nullptr;

	if (IsValid(MeshComponent))
	{
		MeshComponent->DestroyComponent();
		MeshComponent = nullptr;
	}
}

void UFaerieItemMeshComponent::SetPreferredTag(const FGameplayTag MeshTag)
{
	if (PreferredTag != MeshTag)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PreferredTag, this);
		PreferredTag = MeshTag;
		LoadMeshFromToken(true);
	}
}

void UFaerieItemMeshComponent::SetPreferredMeshType(const EItemMeshType MeshType)
{
	if (PreferredType != MeshType)
	{
		PreferredType = MeshType;
		RebuildMesh();
	}
}

FBoxSphereBounds UFaerieItemMeshComponent::GetBounds() const
{
	switch (ActualType)
	{
	case EItemMeshType::Static: return MeshData.GetStatic()->GetBounds();
	case EItemMeshType::Dynamic: return UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(MeshData.GetDynamic());
	case EItemMeshType::Skeletal:
		{
			if (CacheSkeletalBoundsInPose && CachedBounds.IsSet())
			{
				return CachedBounds.GetValue();
			}
			return MeshData.GetSkeletal().Mesh->GetBounds();
		}
	default: return FBoxSphereBounds();
	}
}