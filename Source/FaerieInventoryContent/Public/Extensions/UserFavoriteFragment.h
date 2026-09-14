// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieMassFragment.h"
#include "Actions/FaerieClientActionBase.h"
#include "UserFavoriteFragment.generated.h"

/*
 * Sparse tag fragment to mark an instance as a user favorite. UI can filter by this tag's presence.
 * Blueprint can look for this with HasItemFragment.
 */
USTRUCT()
struct FFaerieUserFavoriteFragment : public FFaerieMassSparseTag
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_FavoriteItem final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MarkStackWithTag")
	FFaerieItemNetworkHandle Handle;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_UnfavoriteItem final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(TNotNull<const UFaerieInventoryClient*> Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "ClearTagFromStack")
	FFaerieItemNetworkHandle Handle;
};