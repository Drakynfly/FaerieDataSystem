// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "CraftingActionConfig.generated.h"

/**
* Base class for configuration objects passed into UFaerieItemCraftingSubsystem functions.
*/
UCLASS(Abstract, Const, DefaultToInstanced, BlueprintType, ClassGroup = "Faerie", CollapseCategories)
class FAERIEITEMGENERATOR_API UCraftingActionConfig : public UObject
{
	GENERATED_BODY()

public:
	// Generation drivers need to be replicate-able, so that runtime created drivers can be requested to run by clients.
	// Crafting and upgrade configs both should only be sourced from cooked asset data, which should enable replication,
	// but to be safe, networking is forced 'on' here.
	// None of our members need to replicate however, as they should only be set/used by the server on creation.
	virtual bool IsSupportedForNetworking() const override { return true; }
};