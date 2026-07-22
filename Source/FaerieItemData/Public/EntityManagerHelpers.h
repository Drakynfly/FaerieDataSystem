// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassEntityManager.h"

namespace Faerie::ItemData
{
#if WITH_EDITOR
	bool IsWorldContextInEditor(TNotNull<UObject*> Obj);

#define CHECK_NOT_CALLED_IN_EDITOR(Obj) if (::Faerie::ItemData::IsWorldContextInEditor(Obj)) { check(0); }
#else
#define CHECK_NOT_CALLED_IN_EDITOR(Obj)
#endif

	/*
	 * Get the Mass Entity Manager used for all Faerie Item Fragment operations.
	 */
	FAERIEITEMDATA_API FMassEntityManager* GetFaerieEntityManager();
	FAERIEITEMDATA_API FMassEntityManager& GetFaerieEntityManagerChecked();

	/*
	 * Assign the Mass Entity Manager to be used for all Faerie Item Fragment operations.
	 * This should be called once during initialization of subsystems, before the first BeginPlay.
	 */
	FAERIEITEMDATA_API void SetFaerieEntityManager(FMassEntityManager& EntityManager);

	/* A utility for APIs that require a MassEntityManager. This reduces the boilerplate of retrieving the manager from
	 * the world, while also able to use one provided, if available.
	 */
	struct [[nodiscard]] FAERIEITEMDATA_API FRequireEntityManager
	{
		UE_NONCOPYABLE(FRequireEntityManager)

		// Ctor to use an existing reference.
		FRequireEntityManager(FMassEntityManager& EntityManager UE_LIFETIMEBOUND)
		  : EntityManager(EntityManager) {}

		// Ctor to fetch the manager from the world.
		explicit FRequireEntityManager(const TNotNull<const UObject*> WorldContextObject);

		UE_REWRITE FMassEntityManager& Resolve() const { return EntityManager; }

		UE_REWRITE FMassEntityManager& operator*() const { return Resolve(); }
		UE_REWRITE FMassEntityManager* operator->() const { return &EntityManager; }

	private:
		FMassEntityManager& EntityManager;
	};

	/* A utility for APIs that can optionally use a MassEntityManager. This reduces the boilerplate of retrieving the manager from
	 * the world, while also able to use one provided, if available.
	 */
	struct [[nodiscard]] FAERIEITEMDATA_API FOptionalEntityManager
	{
		UE_NONCOPYABLE(FOptionalEntityManager)

		// Ctor to use an existing reference.
		explicit FOptionalEntityManager(FMassEntityManager& EntityManager UE_LIFETIMEBOUND)
		  : EntityManager(&EntityManager) {}

		// Ctor to fetch the manager from the world.
		explicit FOptionalEntityManager(const TNotNull<const UObject*> WorldContextObject);

		FOptionalEntityManager(const FRequireEntityManager& FromRequired)
		  : EntityManager(FromRequired.operator->()) {}

		UE_REWRITE FMassEntityManager* ResolvePtr() const { return EntityManager; }
		UE_REWRITE FMassEntityManager* operator->() const { return ResolvePtr(); }

	private:
		FMassEntityManager* EntityManager;
	};
}
