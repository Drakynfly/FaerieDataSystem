// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMeshSubsystem.h"
#include "FaerieItemMeshLoader.h"
#include "FaerieMeshSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMeshSubsystem)

bool UFaerieMeshSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	auto&& MeshSettings = GetDefault<UFaerieMeshSettings>();
	return Super::ShouldCreateSubsystem(Outer) && MeshSettings->CreateMeshLoaderWorldSubsystem;
}

void UFaerieMeshSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Loader = NewObject<UFaerieItemMeshLoader_Cached>(this);
}

bool UFaerieMeshSubsystem::LoadMeshFromTokenSynchronous(const UFaerieMeshTokenBase* Token, FGameplayTag Purpose,
														FFaerieItemMesh& Mesh)
{
	// This is a stupid fix for an issue with blueprints, where impure nodes will cache their output across multiple executions.
	// The result without this line, is that a mesh value put into Mesh in one call will persist for successive calls.
	Mesh = FFaerieItemMesh();

	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
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
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	return Loader->LoadMeshFromProxySynchronous(Proxy, Purpose, Mesh);
}

void UFaerieMeshSubsystem::LoadMeshFromTokenAsynchronous(const UFaerieMeshTokenBase* Token, FGameplayTag Purpose,
														 const FFaerieItemMeshAsyncLoadResult& Callback)
{
	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	Loader->LoadMeshFromTokenAsynchronous(Token, Purpose,
		Faerie::FItemMeshAsyncLoadResult::CreateLambda(
			[Callback](const bool Success, FFaerieItemMesh&& Mesh)
			{
				(void)Callback.ExecuteIfBound(Success, Mesh);
			}));
}

void UFaerieMeshSubsystem::LoadMeshFromTokenAsynchronous(const UFaerieMeshTokenBase* Token, FGameplayTag Purpose,
	const Faerie::FItemMeshAsyncLoadResult& Callback)
{
	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	Loader->LoadMeshFromTokenAsynchronous(Token, Purpose, Callback);
}

void UFaerieMeshSubsystem::LoadMeshFromProxyAsynchronous(const FFaerieItemProxy Proxy, FGameplayTag Purpose,
														 const FFaerieItemMeshAsyncLoadResult& Callback)
{
	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	Loader->LoadMeshFromProxyAsynchronous(Proxy, Purpose,
		Faerie::FItemMeshAsyncLoadResult::CreateLambda(
			[Callback](const bool Success, FFaerieItemMesh&& Mesh)
			{
				(void)Callback.ExecuteIfBound(Success, Mesh);
			}));
}

void UFaerieMeshSubsystem::LoadMeshFromProxyAsynchronous(const FFaerieItemProxy Proxy, FGameplayTag Purpose,
	const Faerie::FItemMeshAsyncLoadResult& Callback)
{
	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	Loader->LoadMeshFromProxyAsynchronous(Proxy, Purpose, Callback);
}
