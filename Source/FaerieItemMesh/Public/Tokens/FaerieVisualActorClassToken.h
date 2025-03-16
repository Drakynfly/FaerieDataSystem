// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "Actors/ItemRepresentationActor.h"

#include "FaerieVisualActorClassToken.generated.h"

/**
 *
 */
UCLASS(DisplayName = "Token - Visual: Actor Class")
class FAERIEITEMMESH_API UFaerieVisualActorClassToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	const TSoftClassPtr<AItemRepresentationActor>& GetActorClass() const { return ActorClass; }

	TSubclassOf<AItemRepresentationActor> LoadActorClassSynchronous() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VisualActorClassToken", meta = (ExposeOnSpawn))
	TSoftClassPtr<AItemRepresentationActor> ActorClass;
};