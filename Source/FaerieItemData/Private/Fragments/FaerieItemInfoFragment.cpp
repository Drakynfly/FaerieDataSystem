// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieAssetInfo.h"

FAERIE_REGISTER_TRAITS(FFaerieAssetInfo)

uint32 GetTypeHash(const FFaerieAssetInfo& Value)
{
	return TextKeyUtil::HashString(Value.ObjectName.BuildSourceString());
}
