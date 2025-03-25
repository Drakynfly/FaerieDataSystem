// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMeshSubsystem.h"
#include "FaerieItemMeshLoader.h"

void UFaerieMeshSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Loader = NewObject<UFaerieItemMeshLoader_Cached>(this);
	FallbackPurpose = Faerie::ItemMesh::Tags::MeshPurpose_Default;
}

bool UFaerieMeshSubsystem::LoadMeshFromTokenSynchronous(const UFaerieMeshTokenBase* Token, FGameplayTag Purpose,
														FFaerieItemMesh& Mesh)
{
	// This is a stupid fix for an issue with blueprints, where impure nodes will cache their output across multiple executions.
	// The result without this line, is that a mesh value put into Mesh in one call will persist for successive calls.
	Mesh = FFaerieItemMesh();

	if (!Purpose.IsValid())
	{
		Purpose = FallbackPurpose;
	}

	return Loader->LoadMeshFromTokenSynchronous(Token, Purpose, Mesh);
}

bool UFaerieMeshSubsystem::LoadMeshFromProxySynchronous(const FFaerieItemProxy Proxy, FGameplayTag Purpose,
														FFaerieItemMesh& Mesh)
{
	// This is a stupid fix for an issue with blueprints, where impure nodes will cache their output across multiple executions.
	// The result without this line, is that a mesh value put into Mesh in one call will persist for successive calls.
	Mesh = FFaerieItemMesh();

	if (!Purpose.IsValid())
	{
		Purpose = FallbackPurpose;
	}

	return Loader->LoadMeshFromProxySynchronous(Proxy, Purpose, Mesh);
}

void UFaerieMeshSubsystem::LoadMeshFromTokenAsynchronous(const UFaerieMeshTokenBase* Token, FGameplayTag Purpose,
														 const FFaerieItemMeshAsyncLoadResult& Callback)
{
	if (!Purpose.IsValid())
	{
		Purpose = FallbackPurpose;
	}

	Loader->LoadMeshFromTokenAsynchronous(Token, Purpose,
		Faerie::FItemMeshAsyncLoadResult::CreateLambda(
			[Callback](const bool Success, const FFaerieItemMesh& Mesh)
			{
				(void)Callback.ExecuteIfBound(Success, Mesh);
			}));
}

void UFaerieMeshSubsystem::LoadMeshFromProxyAsynchronous(const FFaerieItemProxy Proxy, FGameplayTag Purpose,
														 const FFaerieItemMeshAsyncLoadResult& Callback)
{
	if (!Purpose.IsValid())
	{
		Purpose = FallbackPurpose;
	}

	Loader->LoadMeshFromProxyAsynchronous(Proxy, Purpose,
		Faerie::FItemMeshAsyncLoadResult::CreateLambda(
			[Callback](const bool Success, const FFaerieItemMesh& Mesh)
			{
				(void)Callback.ExecuteIfBound(Success, Mesh);
			}));
}
