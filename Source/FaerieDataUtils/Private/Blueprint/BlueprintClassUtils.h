// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintClassUtils.generated.h"

/**
 *
 */
UCLASS()
class UBlueprintClassUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Traverse the outer chain until it finds a package directly associated with the object.
	UFUNCTION(BlueprintPure, Category = "Faerie|Utils")
	static UPackage* GetPackage(const UObject* Object);

	// Traverse the outer chain to find the first Outer of a given class (or nullptr if there isn't one).
	UFUNCTION(BlueprintCallable, Category = "Faerie|Utils", meta = (DeterminesOutputType = Class))
	static UObject* GetTypedOuter(const UObject* Object, TSubclassOf<UObject> Class);

	// Traverse the attached parent chain to find the first Parent of a given class (or nullptr if there isn't one).
	UFUNCTION(BlueprintCallable, Category = "Faerie|Utils", meta = (DeterminesOutputType = Class))
	static USceneComponent* GetTypedParent(const USceneComponent* Component, TSubclassOf<USceneComponent> Class);

	// @todo this function is a duplicate to the one in UHeartGeneralUtils, but this plugin needs one too.
	// Hopefully, Epic will expose this natively at some point, so i can get rid of both

	// Adds an object to the actor's SubObjectList so it can be replicated.
	// The actor must be somewhere up the objects outer chain, and have ReplicateUsingRegisteredSubObjectList enabled
	UFUNCTION(BlueprintCallable, Category = "Faerie|Utils", meta = (DefaultToSelf = Actor))
	static bool AddReplicatedSubObject(AActor* Actor, UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Utils", meta = (DefaultToSelf = Actor))
	static bool RemoveReplicatedSubObject(AActor* Actor, UObject* Object);
};