// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EntityManagerHelpers.h"
#include "Misc/AssertionMacros.h"

namespace Faerie::ItemData
{
	// Ugly global state, but the alternative is every *single* API that deals with Faerie Items needing to pass down
	// either a MassEntityManager or a WorldContextObj that can resolve to one. The end result would be every function
	// passing the same value anyway, so pretend this is just a hidden parameter on every function in this plugin :)
	static FMassEntityManager* GFaerieEntityManagerPtr = nullptr;

	bool HasFaerieEntityManagerBeenAssigned()
	{
		return !!GFaerieEntityManagerPtr;
	}

	FMassEntityManager* GetFaerieEntityManager()
	{
		return GFaerieEntityManagerPtr;
	}

	FMassEntityManager& GetFaerieEntityManagerChecked()
	{
		check(GFaerieEntityManagerPtr);
		return *GFaerieEntityManagerPtr;
	}

	void SetFaerieEntityManager(FMassEntityManager* EntityManager)
	{
		GFaerieEntityManagerPtr = EntityManager;
	}
}
