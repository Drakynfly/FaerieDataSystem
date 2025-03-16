// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieVisualEquipment.generated.h"

class AItemRepresentationActor;

/**
 * #DEPRECATED USE UFaerieVisualActorClassToken instead
 */
UCLASS(meta = (DisplayName = "Token - Visual: Equipment (DEPRECATED)"))
class FAERIEEQUIPMENT_API UFaerieVisualEquipment : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	const TSoftClassPtr<AItemRepresentationActor>& GetActorClass() const { return ActorClass; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faerie|VisualEquipment", meta = (ExposeOnSpawn))
	TSoftClassPtr<AItemRepresentationActor> ActorClass;
};