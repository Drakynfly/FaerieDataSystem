// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerEvent)

namespace Faerie::Container
{
	bool FEvent::IsAdditionEvent() const
	{
		return Type == Inventory::Tags::Addition;
	}

	bool FEvent::IsRemovalEvent() const
	{
		return Type.MatchesTag(Inventory::Tags::RemovalBase);
	}

	bool FEvent::IsEditEvent() const
	{
		return Type.MatchesTag(Inventory::Tags::EditBase);
	}

	bool FEvent::IsReplicationEvent() const
	{
		return Type == Inventory::Tags::ReplicationEdit;
	}
}
