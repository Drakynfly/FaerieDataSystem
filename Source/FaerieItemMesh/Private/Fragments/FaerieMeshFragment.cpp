// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieMeshFragment.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMeshFragment)

FAERIE_REGISTER_TRAITS(FFaerieMeshFragment)

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieMeshFragmentValidation"

EDataValidationResult FFaerieMeshFragment::IsDataValid(FDataValidationContext& Context) const
{
	for (auto&& i : Container.StaticMeshes)
	{
		if (i.StaticMesh.IsNull())
		{
			Context.AddError(LOCTEXT("IsDataValid_Failed_InvalidStaticMesh", "Invalid static mesh found"));
		}
	}

	for (auto&& i : Container.SkeletalMeshes)
	{
		if (i.SkeletonAndAnimation.Mesh.IsNull())
		{
			Context.AddError(LOCTEXT("IsDataValid_Failed_InvalidSkeletalMesh", "Invalid skeletal mesh found"));
		}
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE

#endif