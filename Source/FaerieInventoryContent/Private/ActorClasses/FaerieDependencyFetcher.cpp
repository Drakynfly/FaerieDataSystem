// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ActorClasses/FaerieDependencyFetcher.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieDependencyFetcher)

UFaerieDependencyFetcher::UFaerieDependencyFetcher()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFaerieDependencyFetcher::FetchDependency_Implementation(const TSubclassOf<UActorComponent> Class,
															  UActorComponent*& Component) const
{
	return GetDependency(Class, Component);
}

bool UFaerieDependencyFetcher::GetDependency_Implementation(const TSubclassOf<UActorComponent> Class,
															UActorComponent*& Component) const
{
	AActor* Owner = GetOwner();
	if (IsValid(Owner))
	{
		Component = Owner->FindComponentByClass(Class);
		if (IsValid(Component))
		{
			return true;
		}
	}

	return false;
}