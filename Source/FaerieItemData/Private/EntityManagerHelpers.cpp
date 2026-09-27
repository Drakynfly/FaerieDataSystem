// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EntityManagerHelpers.h"
#include "MassEntitySubsystem.h"

#include "Engine/World.h"

namespace Faerie::ItemData
{
	bool HasFaerieEntityManagerBeenAssigned(const TNotNull<const UWorld*> World)
	{
		return World->HasSubsystem<UMassEntitySubsystem>();
	}

	FMassEntityManager* GetFaerieEntityManager(const TNotNull<const UWorld*> World)
	{
		return &World->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();
	}

	FMassEntityManager& GetFaerieEntityManagerChecked(const TNotNull<const UWorld*> World)
	{
		return World->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();
	}
}
