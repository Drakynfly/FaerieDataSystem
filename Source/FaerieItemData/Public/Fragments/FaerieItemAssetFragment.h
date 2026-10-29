// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieItemAssetFragment.generated.h"

/*
 * Const Shared fragment to keep a reference to a faerie item object.
 */
USTRUCT()
struct FFaerieMassItemObject : public FFaerieMassConstSharedFragment
{
	GENERATED_BODY()

	FFaerieMassItemObject() = default;
	FFaerieMassItemObject(const UFaerieItemAsset* Item) : ItemAsset(Item) {}

	UPROPERTY(VisibleAnywhere, Category = "MassItemObject")
	TObjectPtr<const UFaerieItemAsset> ItemAsset;

	bool InitializeRuntime(UE::Mass::FEntityBuilder& Builder);

____FAERIE_FRAGMENT_DECL(FFaerieMassItemObject)
};

// Item Asset used for "default" values.
USTRUCT()
struct FFaerieItemDataDefaults : public FFaerieMassItemObject
{
	GENERATED_BODY()
	using FFaerieMassItemObject::FFaerieMassItemObject;
____FAERIE_FRAGMENT_DECL(FFaerieItemDataDefaults)
};