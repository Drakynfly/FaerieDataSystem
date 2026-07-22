// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryUserdataExtension.h"

#include "FaerieItemStorage.h"
#include "Actions/FaerieInventoryClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryUserdataExtension)

using namespace Faerie;

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryUserTag, Favorite,
		"Fae.Inventory.Public.Favorite", "Marks an item to show up in player favorites / quick access.");
}

UScriptStruct* UInventoryUserdataExtension::GetDataScriptStruct() const
{
	return FFaerieStorageEntryUserdata::StaticStruct();
}

bool UInventoryUserdataExtension::DoesStackHaveTag(const UFaerieItemContainerBase* Container,
	const FFaerieAddress Address, const FFaerieInventoryUserTag Tag) const
{
	const FConstStructView AddressData = GetDataForHandle(Container, Address);
	if (!AddressData.IsValid())
	{
		return false;
	}

	return AddressData.Get<const FFaerieStorageEntryUserdata>().Tags.HasTag(Tag);
}

bool UInventoryUserdataExtension::CanSetStackTag(const UFaerieItemContainerBase* Container,
	const FFaerieAddress Address, const FFaerieInventoryUserTag Tag, const bool StateToSetTo) const
{
	return DoesStackHaveTag(Container, Address, Tag) != StateToSetTo;
}

bool UInventoryUserdataExtension::MarkStackWithTag(const TNotNull<const UFaerieItemContainerBase*> Container,
	const FFaerieAddress Address, const FFaerieInventoryUserTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetStackTag(Container, Address, Tag, true))
	{
		return false;
	}

	return EditDataForHandle(Container, Address,
		[Tag](const FStructView Data)
		{
			Data.Get<FFaerieStorageEntryUserdata>().Tags.AddTag(Tag);
		});
}

bool UInventoryUserdataExtension::ClearTagFromStack(const TNotNull<const UFaerieItemContainerBase*> Container,
	const FFaerieAddress Address, const FFaerieInventoryUserTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetStackTag(Container, Address, Tag, false))
	{
		return false;
	}

	return EditDataForHandle(Container, Address,
		[Tag](const FStructView Data)
		{
			Data.Get<FFaerieStorageEntryUserdata>().Tags.RemoveTag(Tag);
		});
}

bool FFaerieClientAction_MarkStackWithTag::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	auto&& Container = Handle.Container.Get();
	if (!IsValid(Container)) return false;
	if (!Client->CanAccessContainer(Container, StaticStruct())) return false;

	if (auto&& Userdata = Extensions::Get<UInventoryUserdataExtension>(Container->GetExtensions(), true))
	{
		return Userdata->MarkStackWithTag(Handle.Container.Get(), Handle.Address, Tag);
	}
	return false;
}

bool FFaerieClientAction_ClearTagFromStack::Server_Execute(const TNotNull<const UFaerieInventoryClient*> Client) const
{
	auto&& Storage = Handle.Container.Get();
	if (!IsValid(Storage)) return false;
	if (!Client->CanAccessContainer(Storage, StaticStruct())) return false;

	if (auto&& Userdata = Extensions::Get<UInventoryUserdataExtension>(Storage->GetExtensions(), true))
	{
		return Userdata->ClearTagFromStack(Handle.Container.Get(), Handle.Address, Tag);
	}
	return false;
}