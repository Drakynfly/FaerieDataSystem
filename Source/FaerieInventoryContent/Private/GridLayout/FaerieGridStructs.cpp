// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GridLayout/FaerieGridStructs.h"
#include "GridLayout/InventoryGridExtensionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieGridStructs)

void FFaerieGridKeyedStack::PreReplicatedRemove(const FFaerieGridContent& InArraySerializer)
{
	InArraySerializer.PreStackReplicatedRemove(*this);
}

void FFaerieGridKeyedStack::PostReplicatedAdd(const FFaerieGridContent& InArraySerializer)
{
	InArraySerializer.PostStackReplicatedAdd(*this);
}

void FFaerieGridKeyedStack::PostReplicatedChange(const FFaerieGridContent& InArraySerializer)
{
	InArraySerializer.PostStackReplicatedChange(*this);
}

FFaerieGridContent::FScopedStackHandle::FScopedStackHandle(const FFaerieAddress Key, FFaerieGridContent& Source)
  : Handle(Source.Items[Source.IndexOf(Key)]),
	Source(Source)
{
#if FAERIE_DEBUG
	Source.WriteLock++;
#endif
}

FFaerieGridContent::FScopedStackHandle::~FScopedStackHandle()
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(Source.WriteLock > 0);
	}
	Source.WriteLock--;
#endif

	// Propagate change to client
	Source.MarkItemDirty(Handle);

	// Broadcast change on server
	Source.PostStackReplicatedChange(Handle);
}

void FFaerieGridContent::PreStackReplicatedRemove(const FFaerieGridKeyedStack& Stack) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PreStackRemove_Client(Stack);
	}
}

void FFaerieGridContent::PostStackReplicatedAdd(const FFaerieGridKeyedStack& Stack) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PostStackAdd(Stack);
	}
}

void FFaerieGridContent::PostStackReplicatedChange(const FFaerieGridKeyedStack& Stack) const
{
	if (IsValid(ChangeListener))
	{
		ChangeListener->PostStackChange(Stack);
	}
}

void FFaerieGridContent::Insert(FFaerieAddress Key, const FFaerieGridPlacement& Value)
{
	check(Key.IsValid())
#if FAERIE_DEBUG
	check(WriteLock == 0);
#endif

	FFaerieGridKeyedStack& NewStack = BSOA::Insert({Key, Value});

	PostStackReplicatedAdd(NewStack);
	MarkItemDirty(NewStack);
}

void FFaerieGridContent::Remove(const FFaerieAddress Key)
{
	check(Key.IsValid())
#if FAERIE_DEBUG
	check(WriteLock == 0);
#endif

	if (BSOA::Remove(Key))
	{
		// Notify clients of this removal.
		MarkArrayDirty();
	}
}

FFaerieGridContent::TRangedForConstIterator FFaerieGridContent::begin() const
{
#if FAERIE_DEBUG
	WriteLock++;
#endif
	return TRangedForConstIterator(Items.begin());
}

FFaerieGridContent::TRangedForConstIterator FFaerieGridContent::end() const
{
#if FAERIE_DEBUG
	if (Faerie::Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
	{
		ensureAlways(WriteLock > 0);
	}
	WriteLock--;
#endif
	return TRangedForConstIterator(Items.end());
}
