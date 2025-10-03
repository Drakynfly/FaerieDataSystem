// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieVisualActorClassToken.generated.h"

class AFaerieItemOwningActorBase;
class AFaerieProxyActorBase;

/**
 *
 */
UCLASS(DisplayName = "Token - Visual: Actor Class")
class FAERIEITEMMESH_API UFaerieVisualActorClassToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	const TSoftClassPtr<AFaerieItemOwningActorBase>& GetOwningActorClass() const { return OwningActorClass; }
	const TSoftClassPtr<AFaerieProxyActorBase>& GetProxyActorClass() const { return ProxyActorClass; }

	TSubclassOf<AFaerieItemOwningActorBase> LoadOwningActorClassSynchronous() const;
	TSubclassOf<AFaerieProxyActorBase> LoadProxyActorClassSynchronous() const;

protected:
	// Actor class to use when spawning an actor to own this item (e.g. pickups)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VisualActorClassToken", meta = (ExposeOnSpawn))
	TSoftClassPtr<AFaerieItemOwningActorBase> OwningActorClass;

	// Actor class to use when spawning proxy visualizations of this item (e.g. equipment)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VisualActorClassToken", meta = (ExposeOnSpawn))
	TSoftClassPtr<AFaerieProxyActorBase> ProxyActorClass;
};