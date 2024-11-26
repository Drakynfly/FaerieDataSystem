﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieGridStructs.h"
#include "Extensions/InventoryGridExtensionBase.h"

void FFaerieGridKeyedStack::PreReplicatedRemove(const FFaerieGridContent& InArraySerializer)
{
	InArraySerializer.PreStackReplicatedRemove(*this);
}

void FFaerieGridKeyedStack::PostReplicatedAdd(FFaerieGridContent& InArraySerializer)
{
	InArraySerializer.PostStackReplicatedAdd(*this);
}

void FFaerieGridKeyedStack::PostReplicatedChange(const FFaerieGridContent& InArraySerializer)
{
	InArraySerializer.PostStackReplicatedChange(*this);
}

bool FFaerieGridContent::EditItem(const FInventoryKey Key, const TFunctionRef<bool(FFaerieGridPlacement&)>& Func)
{
	if (const int32 Index = IndexOf(Key);
		Index != INDEX_NONE)
	{
		if (FFaerieGridKeyedStack& Stack = Items[Index];
			Func(Stack.Value))
		{
			MarkItemDirty(Stack);
			PostStackReplicatedChange(Stack);
			return true;
		}
	}
	return false;
}

void FFaerieGridContent::PreStackReplicatedRemove(const FFaerieGridKeyedStack& Stack) const
{
	if (ChangeListener.IsValid())
	{
		ChangeListener->PreStackRemove_Client(Stack);
	}
}

void FFaerieGridContent::PostStackReplicatedAdd(const FFaerieGridKeyedStack& Stack)
{
	if (ChangeListener.IsValid())
	{
		ChangeListener->PostStackAdd(Stack);
	}
}

void FFaerieGridContent::PostStackReplicatedChange(const FFaerieGridKeyedStack& Stack) const
{
	if (ChangeListener.IsValid())
	{
		ChangeListener->PostStackChange(Stack);
	}
}

void FFaerieGridContent::Insert(FInventoryKey Key, const FFaerieGridPlacement& Value)
{
	check(Key.IsValid())

	FFaerieGridKeyedStack& NewStack = BSOA::Insert({Key, Value});

	PostStackReplicatedAdd(NewStack);
	MarkItemDirty(NewStack);
}

void FFaerieGridContent::Remove(const FInventoryKey Key)
{
	if (BSOA::Remove(Key))
	{
		// Notify clients of this removal.
		MarkArrayDirty();
	}
}