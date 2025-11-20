// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AnimUtilityStructs.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"

FSkeletonAndAnimation FSoftSkeletonAndAnimation::LoadSynchronous() const
{
	return FSkeletonAndAnimation{ Mesh.LoadSynchronous(), AnimClass.LoadSynchronous(), AnimationAsset.LoadSynchronous() };
}
