// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryUserdataExtension.h"

#include "FaerieItemStorage.h"
#include "Actions/FaerieInventoryClient.h"

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

bool UInventoryUserdataExtension::DoesStackHaveTag(const FFaerieAddressableHandle Handle, const FFaerieInventoryUserTag Tag) const
{
	const FConstStructView DataView = GetDataForHandle(Handle);
	if (!DataView.IsValid())
	{
		return false;
	}

	return DataView.Get<const FInventoryEntryUserdata>().Tags.HasTag(Tag);
}

bool UInventoryUserdataExtension::CanSetStackTag(const FFaerieAddressableHandle Handle, const FFaerieInventoryUserTag Tag,
                                                  const bool StateToSetTo) const
{
	return DoesStackHaveTag(Handle, Tag) != StateToSetTo;
}

bool UInventoryUserdataExtension::MarkStackWithTag(const FFaerieAddressableHandle Handle, const FFaerieInventoryUserTag Tag)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetStackTag(Handle, Tag, true))
	{
		return false;
	}

	return EditDataForHandle(Handle,
		[Tag](const FStructView Data)
		{
			Data.Get<FInventoryEntryUserdata>().Tags.AddTag(Tag);
		});
}

bool UInventoryUserdataExtension::ClearTagFromStack(const FFaerieAddressableHandle Handle, const FFaerieInventoryUserTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetStackTag(Handle, Tag, false))
	{
		return false;
	}

	return EditDataForHandle(Handle,
		[Tag](const FStructView Data)
		{
			Data.Get<FInventoryEntryUserdata>().Tags.RemoveTag(Tag);
		});
}

bool FFaerieClientAction_MarkStackWithTag::Server_Execute(const UFaerieInventoryClient* Client) const
{
	auto&& Container = Handle.Container.Get();
	if (!IsValid(Container)) return false;
	if (!Client->CanAccessContainer(Container, StaticStruct())) return false;

	if (auto&& Userdata = Faerie::GetExtension<UInventoryUserdataExtension>(Container, true))
	{
		return Userdata->MarkStackWithTag(Handle, Tag);
	}
	return false;
}

bool FFaerieClientAction_ClearTagFromStack::Server_Execute(const UFaerieInventoryClient* Client) const
{
	auto&& Storage = Handle.Container.Get();
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	if (auto&& Userdata = Faerie::GetExtension<UInventoryUserdataExtension>(Storage, true))
	{
		return Userdata->ClearTagFromStack(Handle, Tag);
	}
	return false;
}