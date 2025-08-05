// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Components/FaerieItemMeshComponent.h"
#include "FaerieItemMeshLoader.h"
#include "FaerieMeshStructs.h"
#include "FaerieMeshSubsystem.h"
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "Libraries/FaerieMeshStructsLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Tokens/FaerieMeshToken.h"

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

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SourceMeshToken, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SkeletalMeshLeader, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PreferredTag, SharedParams);
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

void UFaerieItemMeshComponent::LoadMeshFromToken(const bool Async)
{
	if (!IsValid(SourceMeshToken))
	{
		return;
	}

	UFaerieMeshSubsystem* MeshSubsystem = GetWorld()->GetSubsystem<UFaerieMeshSubsystem>();
	if (!IsValid(MeshSubsystem))
	{
#if WITH_EDITOR
		// @todo: this is a hack to allow the editor to load meshes when world subsystems aren't available...
		auto Loader = GetMutableDefault<UFaerieItemMeshLoader>();
		if (Async)
		{
			Loader->LoadMeshFromTokenAsynchronous(SourceMeshToken, PreferredTag,
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
		MeshSubsystem->LoadMeshFromTokenAsynchronous(SourceMeshToken, PreferredTag,
			Faerie::FItemMeshAsyncLoadResult::CreateUObject(this, &ThisClass::AsyncLoadMeshReturn));
	}
	else
	{
		MeshSubsystem->LoadMeshFromTokenSynchronous(SourceMeshToken, PreferredTag, MeshData);
		RebuildMesh();
	}
}

void UFaerieItemMeshComponent::AsyncLoadMeshReturn(const bool Success, const FFaerieItemMesh& InMeshData)
{
	if (Success)
	{
		MeshData = InMeshData;
	}
	else
	{
		MeshData = FFaerieItemMesh();
	}
	RebuildMesh();
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
	if (NewMeshType == EItemMeshType::None && !AllowNullMeshes)
	{
		//@todo log category
		UE_LOG(LogTemp, Error, TEXT("No valid mesh in Mesh Data. RebuildMesh cancelled."))
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

	// Load the mesh and materials to the mesh component.
	switch (ActualType)
	{
	case EItemMeshType::None: break;
	case EItemMeshType::Static:
		if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(MeshComponent))
		{
			StaticMesh->SetStaticMesh(MeshData.GetStatic());
			for (int32 i = 0; i < MeshData.Materials.Num(); ++i)
			{
				StaticMesh->SetMaterial(i, MeshData.Materials[i].Material);
			}

			if (CenterMeshByBounds)
			{
				auto BoundsOrigin = StaticMesh->GetStaticMesh()->GetBounds().Origin;
				StaticMesh->AddLocalOffset(-BoundsOrigin);
			}
		}
		break;
	case EItemMeshType::Dynamic:
		if (UDynamicMeshComponent* DynamicMesh = Cast<UDynamicMeshComponent>(MeshComponent))
		{
			DynamicMesh->SetDynamicMesh(MeshData.GetDynamic());
			DynamicMesh->ConfigureMaterialSet(UFaerieMeshStructsLibrary::FaerieItemMaterialsToObjectArray(MeshData.Materials));

			if (CenterMeshByBounds)
			{
				auto BoundsOrigin = DynamicMesh->GetMesh()->GetBounds().Center();
				DynamicMesh->AddLocalOffset(-BoundsOrigin);
			}
		}
		break;
	case EItemMeshType::Skeletal:
		if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(MeshComponent))
		{
			auto&& Skeletal = MeshData.GetSkeletal();
			SkeletalMesh->SetSkeletalMesh(Skeletal.Mesh);

			// Try to setup animation
			if (IsValid(Skeletal.AnimClass))
			{
				SkeletalMesh->SetAnimInstanceClass(Skeletal.AnimClass);
			}
			else if (IsValid(Skeletal.AnimationAsset))
			{
				SkeletalMesh->PlayAnimation(Skeletal.AnimationAsset, true);
			}
			// If there is no Animation Class or Asset, maybe we were supposed to be a LeaderPose component.
			else if (IsValid(SkeletalMeshLeader))
			{
				SkeletalMesh->SetLeaderPoseComponent(SkeletalMeshLeader, true);
			}
			// No animation, assuming that skeletal mesh is being displayed in default pose. Center if enabled.
			else
			{
				if (CenterMeshByBounds)
				{
					auto BoundsOrigin = SkeletalMesh->GetSkeletalMeshAsset()->GetBounds().Origin;
					SkeletalMesh->AddLocalOffset(-BoundsOrigin);
				}
			}

			// Setup Materials
			for (int32 i = 0; i < MeshData.Materials.Num(); ++i)
			{
				SkeletalMesh->SetMaterial(i, MeshData.Materials[i].Material);
			}
		}
		break;
	default: checkNoEntry();
	}

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
	case EItemMeshType::Skeletal: return MeshData.GetSkeletal().Mesh->GetBounds();
	default: return FBoxSphereBounds();
	}
}