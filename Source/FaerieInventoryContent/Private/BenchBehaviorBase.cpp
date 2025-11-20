// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BenchBehaviorBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BenchBehaviorBase)

UUserWidget* UBenchBehaviorBase::MakeInteractionWidget_Implementation(APlayerController* PlayerController)
{
	return nullptr;
}

bool UBenchBehaviorBase::CanPlayerInteract() const
{
	if (ExternalInteractionTest.IsBound())
	{
		return ExternalInteractionTest.Execute();
	}

	return DefaultInteractionTest();
}

void UBenchBehaviorBase::SetInteractionTest(const FBenchInteractionTest& InteractionTest)
{
	ExternalInteractionTest = InteractionTest;
}

void UBenchBehaviorBase::NotifyInteractBegin(APlayerController* RequestingPlayer)
{
	BeginInteraction();
	OnInteractionBegin.Broadcast();
}

void UBenchBehaviorBase::NotifyInteractEnd(APlayerController* RequestingPlayer)
{
	EndInteraction();
	OnInteractionEnd.Broadcast();
}