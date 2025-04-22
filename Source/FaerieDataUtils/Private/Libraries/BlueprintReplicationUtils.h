// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintReplicationUtils.generated.h"

/**
 *
 */
UCLASS()
class UBlueprintReplicationUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// @todo this function is a duplicate to the one in UHeartGeneralUtils, but this plugin needs one too.
	// Hopefully, Epic will expose this natively at some point, so i can get rid of both

	// Adds an object to the actor's SubObjectList so it can be replicated.
	// The actor must be somewhere up the objects outer chain, and have ReplicateUsingRegisteredSubObjectList enabled
	UFUNCTION(BlueprintCallable, Category = "Faerie|Utils", meta = (DefaultToSelf = Actor))
	static bool AddReplicatedSubObject(AActor* Actor, UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Utils", meta = (DefaultToSelf = Actor))
	static bool RemoveReplicatedSubObject(AActor* Actor, UObject* Object);
};