// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Components/FaerieItemMeshComponent.h"
#include "FaerieMeshStructs.h"
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "Libraries/FaerieMeshStructsLibrary.h"

UFaerieItemMeshComponent::UFaerieItemMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PreferredType = EItemMeshType::Static;
	ActualType = EItemMeshType::None;
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
		switch (ActualType)
		{
		case EItemMeshType::None: break;
		case EItemMeshType::Static:
			MeshComponent = NewObject<UStaticMeshComponent>(GetOwner());
			break;
		case EItemMeshType::Dynamic:
			MeshComponent = NewObject<UDynamicMeshComponent>(GetOwner());
			break;
		case EItemMeshType::Skeletal:
			MeshComponent = NewObject<USkeletalMeshComponent>(GetOwner());
			break;
		default: checkNoEntry();
		}

		MeshComponent->RegisterComponent();

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
				StaticMesh->SetMaterial(i, MeshData.Materials[i].Material.LoadSynchronous());
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
			SkeletalMesh->SetSkeletalMesh(Skeletal.Mesh);
			if (IsValid(Skeletal.AnimClass))
			{
				SkeletalMesh->SetAnimInstanceClass(Skeletal.AnimClass);
			}
			else
			{
				// If there is no AnimClass, maybe we were supposed to be a LeaderPose component. Check if our parent supports this
				if (USkinnedMeshComponent* ParentMesh = Cast<USkinnedMeshComponent>(GetAttachParent()))
				{
					SkeletalMesh->SetLeaderPoseComponent(ParentMesh, true);
				}
			}
			for (int32 i = 0; i < MeshData.Materials.Num(); ++i)
			{
				SkeletalMesh->SetMaterial(i, MeshData.Materials[i].Material.LoadSynchronous());
			}
		}
		break;
	default: checkNoEntry();
	}
}

void UFaerieItemMeshComponent::SetItemMesh(const FFaerieItemMesh& InMeshData)
{
	if (MeshData != InMeshData)
	{
		MeshData = InMeshData;
		RebuildMesh();
	}
}

void UFaerieItemMeshComponent::ClearItemMesh()
{
	ActualType = EItemMeshType::None;
	MeshData = FFaerieItemMesh();

	if (IsValid(MeshComponent))
	{
		MeshComponent->DestroyComponent();
		MeshComponent = nullptr;
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