// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Components/BenchBehaviorBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BenchBehaviorBase)

void UFaerieBenchComponentBase::NotifyInteractBegin(APlayerController* RequestingPlayer)
{
	BeginInteraction();
}

void UFaerieBenchComponentBase::NotifyInteractEnd(APlayerController* RequestingPlayer)
{
	EndInteraction();
}