// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

struct FMassEntityManager;

namespace Faerie::ItemData
{
	/*
	 * Get the Mass Entity Manager used for all Faerie Item Fragment operations.
	 */
	FAERIEITEMDATA_API bool HasFaerieEntityManagerBeenAssigned();
	FAERIEITEMDATA_API FMassEntityManager* GetFaerieEntityManager();
	FAERIEITEMDATA_API FMassEntityManager& GetFaerieEntityManagerChecked();

	/*
	 * Assign the Mass Entity Manager to be used for all Faerie Item Fragment operations.
	 * This should be called once during initialization of subsystems, before the first BeginPlay.
	 */
	FAERIEITEMDATA_API void SetFaerieEntityManager(FMassEntityManager* EntityManager);
}
