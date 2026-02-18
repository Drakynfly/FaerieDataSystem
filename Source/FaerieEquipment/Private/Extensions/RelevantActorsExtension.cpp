// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/RelevantActorsExtension.h"
#include "FaerieItemContainerBase.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RelevantActorsExtension)

namespace Faerie::Finders
{
	APawn* GetPawn(AController* Controller)
	{
		return Controller->GetPawn();
	}

	APawn* GetPawn(APlayerState* PlayerState)
	{
		return PlayerState->GetPawn();
	}

	AController* GetController(APawn* Pawn)
	{
		return Pawn->Controller;
	}

	AController* GetController(APlayerState* PlayerState)
	{
		return PlayerState->GetOwningController();
	}

	APlayerState* GetPlayerState(APawn* Pawn)
	{
		return Pawn->GetPlayerState();
	}

	APlayerState* GetPlayerState(AController* Controller)
	{
		return Controller->PlayerState;
	}
}

void URelevantActorsExtension::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	if (AActor* Owner = Container->GetTypedOuter<AActor>())
	{
		int32* Counter = OwningActors.Find(Owner);
		if (Counter)
		{
			(*Counter)++;
		}
		else
		{
			RelevantActors.Add(Owner);
			OwningActors.Add(Owner, 1);
		}
	}
}

void URelevantActorsExtension::DeinitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	if (AActor* Owner = Container->GetTypedOuter<AActor>())
	{
		int32* Counter = OwningActors.Find(Owner);
		if (Counter)
		{
			if (*Counter == 1)
			{
				OwningActors.Remove(Owner);
				RelevantActors.Remove(Owner);
			}
			else
			{
				(*Counter)--;
			}
		}
	}
}

AActor* URelevantActorsExtension::FindActor(const TSubclassOf<AActor> Class) const
{
	if (!IsValid(Class)) return nullptr;

	const bool FindingPawn = Class->IsChildOf<APawn>();
	const bool FindingController = Class->IsChildOf<AController>();
	const bool FindingPlayerState = Class->IsChildOf<APlayerState>();
	const bool FindingThroughOther = FindingPawn | FindingController | FindingPlayerState;

	for (auto&& Actor : RelevantActors)
	{
		if (!Actor.IsValid()) continue;

		if (Actor->IsA(Class))
		{
			return Actor.Get();
		}

		if (FindingThroughOther)
		{
			if (APawn* AsPawn = Cast<APawn>(Actor))
			{
				if (FindingController) return Faerie::Finders::GetController(AsPawn);
				if (FindingPlayerState) return Faerie::Finders::GetPlayerState(AsPawn);
			}
			else if (APlayerState* AsPlayerState = Cast<APlayerState>(Actor))
			{
				if (FindingPawn) return Faerie::Finders::GetPawn(AsPlayerState);
				if (FindingController) return Faerie::Finders::GetController(AsPlayerState);
			}
			else if (AController* AsController = Cast<AController>(Actor))
			{
				if (FindingPawn) return Faerie::Finders::GetPawn(AsController);
				if (FindingPlayerState) return Faerie::Finders::GetPlayerState(AsController);
			}
		}
	}

	return nullptr;
}

void URelevantActorsExtension::AddActor(AActor* Actor)
{
	RelevantActors.Add(Actor);
}

void URelevantActorsExtension::RemoveActor(AActor* Actor)
{
	RelevantActors.Remove(Actor);
}