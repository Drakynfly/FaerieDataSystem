// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieDependencyFetcherInterface.h"
#include "Components/ActorComponent.h"
#include "FaerieDependencyFetcher.generated.h"

/*
 * Base class for dependency injection of components. A child of this class can be found or passed into inventory widgets,
 * where other components can be fetched using this without knowing exactly where they live.
 */
UCLASS(Blueprintable, ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent))
class FAERIEINVENTORYCONTENT_API UFaerieDependencyFetcher : public UActorComponent, public IFaerieDependencyFetcherInterface
{
	GENERATED_BODY()

public:
	UFaerieDependencyFetcher();

	//~
	virtual bool FetchDependency_Implementation(TSubclassOf<UActorComponent> Class, UActorComponent*& Component) const override;
	//~

	UFUNCTION(BlueprintCallable, BlueprintPure = false, BlueprintNativeEvent, Category = "Faerie|DependencyFetcher",
		meta = (ExpandBoolAsExecs = "ReturnValue", DeterminesOutputType = "Class", DynamicOutputParam = "Component"))
	bool GetDependency(TSubclassOf<UActorComponent> Class, UActorComponent*& Component) const;
};