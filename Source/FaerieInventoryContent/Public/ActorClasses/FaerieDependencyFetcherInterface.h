﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "FaerieDependencyFetcherInterface.generated.h"

UINTERFACE(Blueprintable)
class UFaerieDependencyFetcherInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class FAERIEINVENTORYCONTENT_API IFaerieDependencyFetcherInterface
{
	GENERATED_BODY()

public:
	// @note: it appears that DeterminesOutputType is broken for interfaces, BP has to manually cast the return value.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, BlueprintNativeEvent, Category = "Faerie|DependencyFetcher",
		meta = (ExpandBoolAsExecs = "ReturnValue", DeterminesOutputType = "Class", DynamicOutputParam = "Component"))
	bool FetchDependency(TSubclassOf<UActorComponent> Class, UActorComponent*& Component) const;
};