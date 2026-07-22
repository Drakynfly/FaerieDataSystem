// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EntityManagerHelpers.h"
#include "MassEntitySubsystem.h"

#include "Engine/Engine.h"

namespace Faerie::ItemData
{
#if WITH_EDITOR
	bool IsWorldContextInEditor(const TNotNull<UObject*> Obj)
	{
		return Obj->GetWorld()->IsEditorWorld();
	}
#endif

	// Ugly global state, but the alternative is every *single* API that deals with Faerie Items needing to pass down
	// either a MassEntityManager or a WorldContextObj that can resolve to one. The end result would be every function
	// passing the same value anyway, so pretend this is just a hidden parameter on every function in this plugin :)
	FMassEntityManager* GFaerieEntityManagerPtr = nullptr;

	FMassEntityManager* GetFaerieEntityManager()
	{
		return GFaerieEntityManagerPtr;
	}

	FMassEntityManager& GetFaerieEntityManagerChecked()
	{
		return *GFaerieEntityManagerPtr;
	}

	void SetFaerieEntityManager(FMassEntityManager& EntityManager)
	{
		GFaerieEntityManagerPtr = &EntityManager;
	}

	FMassEntityManager& GetMutableEntityManagerChecked(const TNotNull<const UObject*> WorldContextObject)
	{
		return GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert)
			->GetSubsystemChecked<UMassEntitySubsystem>()
				->GetMutableEntityManager();
	}

	FMassEntityManager* GetMutableEntityManagerPtr(const TNotNull<const UObject*> WorldContextObject)
	{
		if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
		{
			if (UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>())
			{
				return &EntitySubsystem->GetMutableEntityManager();
			}
		}
		return nullptr;
	}

	FRequireEntityManager::FRequireEntityManager(const TNotNull<const UObject*> WorldContextObject)
	  : EntityManager(GetMutableEntityManagerChecked(WorldContextObject)) {}

	FOptionalEntityManager::FOptionalEntityManager(const TNotNull<const UObject*> WorldContextObject)
	  : EntityManager(GetMutableEntityManagerPtr(WorldContextObject)) {}
}
