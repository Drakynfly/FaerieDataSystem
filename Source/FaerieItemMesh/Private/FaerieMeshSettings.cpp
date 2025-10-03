// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMeshSettings.h"
#include "FaerieMeshStructs.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMeshSettings)

UFaerieMeshSettings::UFaerieMeshSettings()
{
	FallbackPurpose = Faerie::ItemMesh::Tags::MeshPurpose_Default;
}

FName UFaerieMeshSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}
