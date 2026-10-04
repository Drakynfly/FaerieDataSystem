// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EntityManagerHelpers.h"
#include "MassEntitySubsystem.h"

#include "Engine/World.h"

namespace Faerie::ItemData
{
	bool HasFaerieEntityManagerBeenAssigned(const UWorld* World)
	{
		return IsValid(World) && World->HasSubsystem<UMassEntitySubsystem>();
	}

	FMassEntityManager* GetFaerieEntityManager(const UWorld* World)
	{
		if (World)
		{
			if (UMassEntitySubsystem* Subsystem = World->GetSubsystem<UMassEntitySubsystem>())
			{
				return &Subsystem->GetMutableEntityManager();
			}
		}
		return nullptr;
	}

	FMassEntityManager& GetFaerieEntityManagerChecked(const TNotNull<const UWorld*> World)
	{
		return World->GetSubsystem<UMassEntitySubsystem>()->GetMutableEntityManager();
	}
}
