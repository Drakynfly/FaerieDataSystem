// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/NotNull.h"

struct FMassEntityManager;
class UWorld;

namespace Faerie::ItemData
{
	/*
	 * Get the Mass Entity Manager used for all Faerie Item Fragment operations.
	 */
	FAERIEITEMDATA_API bool HasFaerieEntityManagerBeenAssigned(const UWorld* World);
	FAERIEITEMDATA_API FMassEntityManager* GetFaerieEntityManager(const UWorld* World);
	FAERIEITEMDATA_API FMassEntityManager& GetFaerieEntityManagerChecked(TNotNull<const UWorld*> World);
}
