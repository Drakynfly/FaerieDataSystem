// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMeshSubsystem.h"
#include "FaerieItemMeshLoader.h"
#include "FaerieItemMeshLog.h"
#include "FaerieItemProxy.h"
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

	// Create default mesh loader object.
	Loader = NewObject<UFaerieItemMeshLoader_Cached>(this);
}

bool UFaerieMeshSubsystem::LoadMeshFromProxySynchronous(const FFaerieItemProxy& Proxy, FGameplayTag Purpose,
														FFaerieItemMesh& Mesh)
{
	// This is a stupid fix for an issue with blueprints, where impure nodes will cache their output across multiple executions.
	// The result without this line, is that a mesh value put into Mesh in one call will persist for successive calls.
	Mesh = FFaerieItemMesh();

	if (!Proxy.IsValid())
	{
		UE_LOG(LogFaerieItemMesh, Warning, TEXT("Invalid Proxy passed to UFaerieMeshSubsystem::LoadMeshFromProxySynchronous!"))
		return false;
	}

	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	return Loader->LoadMeshFromProxySynchronous(Proxy, Purpose, Mesh);
}

void UFaerieMeshSubsystem::LoadMeshFromProxyAsynchronous(const FFaerieItemProxy& Proxy, FGameplayTag Purpose,
														 const FFaerieItemMeshAsyncLoadResult& Callback)
{
	if (!Proxy.IsValid())
	{
		UE_LOG(LogFaerieItemMesh, Warning, TEXT("Invalid Proxy passed to UFaerieMeshSubsystem::LoadMeshFromProxyAsynchronous!"))
		Callback.Execute(false, FFaerieItemMesh());
		return;
	}

	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	(void)Loader->LoadMeshFromProxyAsynchronous(Proxy, Purpose,
		Faerie::Mesh::FAsyncLoadResult::CreateLambda(
			[Callback](const bool Success, FFaerieItemMesh&& Mesh)
			{
				(void)Callback.ExecuteIfBound(Success, Mesh);
			}));
}

TSharedPtr<FStreamableHandle> UFaerieMeshSubsystem::LoadMeshFromProxyAsynchronous(const FFaerieItemProxy& Proxy, FGameplayTag Purpose,
	const Faerie::Mesh::FAsyncLoadResult& Callback)
{
	if (!Purpose.IsValid())
	{
		Purpose = GetDefault<UFaerieMeshSettings>()->FallbackPurpose;
	}

	return Loader->LoadMeshFromProxyAsynchronous(Proxy, Purpose, Callback);
}
