// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "FaerieActorFragment.generated.h"

class AFaerieProxyActorBase;
class AFaerieItemOwningActorBase;

// Actor class to use when spawning an actor to own this item (e.g. pickups)
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieActorFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ActorFragment")
	TSoftClassPtr<AFaerieItemOwningActorBase> OwningActorClass;

	TSubclassOf<AFaerieItemOwningActorBase> LoadOwningActorClassSynchronous() const;
};

// Actor class to use when spawning proxy visualizations of this item (e.g. equipment)
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FFaerieProxyActorFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ProxyActorFragment")
	TSoftClassPtr<AFaerieProxyActorBase> ProxyActorClass;

	TSubclassOf<AFaerieProxyActorBase> LoadProxyActorClassSynchronous() const;
};