// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCardTags.h"

namespace Faerie
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieItemCardType, CardTypeBase, "Fae.CardType")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieItemCardType, CardType_Full, "Fae.CardType.Full", "A large widget for displaying a highlighted/hovered item")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieItemCardType, CardType_Nameplate, "Fae.CardType.Nameplate", "A small widget for listing many items")
}