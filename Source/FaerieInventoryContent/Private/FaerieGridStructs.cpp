// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieGridStructs.h"
#include "Extensions/InventoryGridExtensionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieGridStructs)

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

FFaerieGridContent::FScopedStackHandle::FScopedStackHandle(const FInventoryKey Key, FFaerieGridContent& Source)
  : Handle(Source.Items[Source.IndexOf(Key)]),
	Source(Source)
{
	Source.WriteLock++;
}

FFaerieGridContent::FScopedStackHandle::~FScopedStackHandle()
{
	Source.WriteLock--;

	// Propagate change to client
	Source.MarkItemDirty(Handle);

	// Broadcast change on server
	Source.PostStackReplicatedChange(Handle);
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
	check(WriteLock == 0);

	FFaerieGridKeyedStack& NewStack = BSOA::Insert({Key, Value});

	PostStackReplicatedAdd(NewStack);
	MarkItemDirty(NewStack);
}

void FFaerieGridContent::Remove(const FInventoryKey Key)
{
	check(WriteLock == 0);

	if (BSOA::Remove(Key))
	{
		// Notify clients of this removal.
		MarkArrayDirty();
	}
}