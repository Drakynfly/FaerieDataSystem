// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryUserdataExtension.h"

#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryUserdataExtension)

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryUserTag, Favorite,
		"Fae.Inventory.Public.Favorite", "Marks an item to show up in player favorites / quick access.");
}

UScriptStruct* UInventoryUserdataExtension::GetDataScriptStruct() const
{
	return FInventoryEntryUserdata::StaticStruct();
}

bool UInventoryUserdataExtension::DoesStackHaveTag(const UFaerieItemContainerBase* Container, const FEntryKey Key, const FFaerieInventoryUserTag Tag) const
{
	const FConstStructView DataView = GetDataForEntry(Container, Key);
	if (!DataView.IsValid())
	{
		return false;
	}

	return DataView.Get<const FInventoryEntryUserdata>().Tags.HasTag(Tag);
}

bool UInventoryUserdataExtension::CanSetStackTag(const UFaerieItemContainerBase* Container, const FEntryKey Key, const FFaerieInventoryUserTag Tag,
                                                  const bool StateToSetTo) const
{
	return DoesStackHaveTag(Container, Key, Tag) != StateToSetTo;
}

bool UInventoryUserdataExtension::MarkStackWithTag(const UFaerieItemContainerBase* Container, const FEntryKey Key, const FFaerieInventoryUserTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetStackTag(Container, Key, Tag, true))
	{
		return false;
	}

	if (!Container->Contains(Key))
	{
		return false;
	}

	return EditDataForEntry(Container, Key,
		[Tag](const FStructView Data)
		{
			Data.Get<FInventoryEntryUserdata>().Tags.AddTag(Tag);
		});
}

bool UInventoryUserdataExtension::ClearTagFromStack(const UFaerieItemContainerBase* Container, const FEntryKey Key, const FFaerieInventoryUserTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetStackTag(Container, Key, Tag, false))
	{
		return false;
	}

	return EditDataForEntry(Container, Key,
		[Tag](const FStructView Data)
		{
			Data.Get<FInventoryEntryUserdata>().Tags.RemoveTag(Tag);
		});
}

bool FFaerieClientAction_MarkStackWithTag::Server_Execute(const UFaerieInventoryClient* Client) const
{
	auto&& Storage = Handle.ItemStorage.Get();
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessStorage(Storage, StaticStruct())) return false;

	if (auto&& Userdata = GetExtension<UInventoryUserdataExtension>(Storage, true))
	{
		return Userdata->MarkStackWithTag(Handle.ItemStorage.Get(), Handle.Key.EntryKey, Tag);
	}
	return false;
}

bool FFaerieClientAction_ClearTagFromStack::Server_Execute(const UFaerieInventoryClient* Client) const
{
	auto&& Storage = Handle.ItemStorage.Get();
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessStorage(Storage, StaticStruct())) return false;

	if (auto&& Userdata = GetExtension<UInventoryUserdataExtension>(Storage, true))
	{
		return Userdata->ClearTagFromStack(Handle.ItemStorage.Get(), Handle.Key.EntryKey, Tag);
	}
	return false;
}