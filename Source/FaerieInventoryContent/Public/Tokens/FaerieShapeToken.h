// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "SpatialTypes.h"
#include "FaerieShapeToken.generated.h"

/**
 * Adds a GridShape to an item. Used with Faerie Spatial Grid Extension.
 */
UCLASS(meta = (DisplayName = "Token - Shape"))
class FAERIEINVENTORYCONTENT_API UFaerieShapeToken : public UFaerieItemToken
{
    GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const FFaerieGridShape& GetShape() const { return Shape; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, meta = (ShowOnlyInnerProperties, ExposeOnSpawn))
	FFaerieGridShape Shape;
};