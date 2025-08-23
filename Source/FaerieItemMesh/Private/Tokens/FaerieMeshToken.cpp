// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieMeshToken.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

void UFaerieMeshTokenBase::GetMeshes(const FGameplayTagContainer& SearchPurposes,
	TConstStructView<FFaerieStaticMeshData>& Static, TConstStructView<FFaerieSkeletalMeshData>& Skeletal) const
{
	Static = GetStaticItemMesh(SearchPurposes);
	Skeletal = GetSkeletalItemMesh(SearchPurposes);
}

void UFaerieMeshTokenBase::GetMeshes(const FGameplayTagContainer& SearchPurposes, bool& FoundStatic,
									 FFaerieStaticMeshData& Static, bool& FoundSkeletal, FFaerieSkeletalMeshData& Skeletal) const
{
	FoundStatic = GetStaticItemMesh(SearchPurposes, Static);
	FoundSkeletal = GetSkeletalItemMesh(SearchPurposes, Skeletal);
}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieMeshTokenValidation"

EDataValidationResult UFaerieMeshToken::IsDataValid(FDataValidationContext& Context) const
{
	for (auto&& i : MeshContainer.StaticMeshes)
	{
		if (i.StaticMesh.IsNull())
		{
			Context.AddError(LOCTEXT("IsDataValid_Failed_InvalidStaticMesh", "Invalid static mesh found"));
		}
	}

	for (auto&& i : MeshContainer.SkeletalMeshes)
	{
		if (i.SkeletonAndAnimation.Mesh.IsNull()
			//|| i.SkeletonAndAnimation.AnimClass.IsNull()
			)
		{
			Context.AddError(LOCTEXT("IsDataValid_Failed_InvalidSkeletalMesh", "Invalid skeletal mesh found"));
		}
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(Context);
}

#undef LOCTEXT_NAMESPACE

#endif

TConstStructView<FFaerieStaticMeshData> UFaerieMeshToken::GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	return MeshContainer.GetStaticItemMesh(SearchPurposes);
}

TConstStructView<FFaerieSkeletalMeshData> UFaerieMeshToken::GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	return MeshContainer.GetSkeletalItemMesh(SearchPurposes);
}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieMeshToken_DynamicValidation"

EDataValidationResult UFaerieMeshToken_Dynamic::IsDataValid(FDataValidationContext& Context) const
{
	for (auto&& Element : DynamicMeshContainer.StaticMeshes)
	{
		for (auto&& Fragment : Element.Fragments)
		{
			if (Fragment.StaticMesh.IsNull())
			{
				Context.AddError(LOCTEXT("IsDataValid_Failed_InvalidStaticMesh", "Invalid static mesh found"));
			}
		}
	}

	for (auto&& Element : DynamicMeshContainer.SkeletalMeshes)
	{
		for (auto&& Fragment : Element.Fragments)
		{
			if (Fragment.SkeletonAndAnimation.Mesh.IsNull()
				//|| Fragment.SkeletonAndAnimation.AnimClass.IsNull()
				)
			{
				Context.AddError(LOCTEXT("IsDataValid_Failed_InvalidSkeletalMesh", "Invalid skeletal mesh found"));
			}
		}
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(Context);
}

#undef LOCTEXT_NAMESPACE

#endif

TConstStructView<FFaerieStaticMeshData> UFaerieMeshToken_Dynamic::GetStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	return TConstStructView<FFaerieStaticMeshData>();
}

TConstStructView<FFaerieSkeletalMeshData> UFaerieMeshToken_Dynamic::GetSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	return TConstStructView<FFaerieSkeletalMeshData>();
}

TConstStructView<FFaerieDynamicStaticMesh> UFaerieMeshToken_Dynamic::GetDynamicStaticItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	return DynamicMeshContainer.GetStaticItemMesh(SearchPurposes);
}

TConstStructView<FFaerieDynamicSkeletalMesh> UFaerieMeshToken_Dynamic::GetDynamicSkeletalItemMesh(const FGameplayTagContainer& SearchPurposes) const
{
	return DynamicMeshContainer.GetSkeletalItemMesh(SearchPurposes);
}