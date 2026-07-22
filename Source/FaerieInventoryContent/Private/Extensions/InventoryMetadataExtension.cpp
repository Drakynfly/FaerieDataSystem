// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryMetadataExtension.h"
#include "Extensions/InventoryEjectionHandlerExtension.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryMetadataExtension)

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryMetaTag, CannotRemove,
		"Fae.Inventory.Meta.CannotRemove", "Denies permission for the user to remove this entry. Typically used to mark required quest items.")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryMetaTag, CannotDelete,
		"Fae.Inventory.Meta.CannotDelete", "Denies permission for the user to delete this entry. Can still be otherwise removed!")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryMetaTag, CannotMove,
		"Fae.Inventory.Meta.CannotMove", "Denies permission for the user to move this entry.")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryMetaTag, CannotEject,
		"Fae.Inventory.Meta.CannotEject", "Denies permission for the user to eject this entry.")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryMetaTag, CannotSplit,
		"Fae.Inventory.Meta.CannotSplit", "Denies permission to split a stack. Typically used to mark required quest item stacks.")
}

using namespace Faerie;

EEventExtensionResponse UInventoryMetadataExtension::AllowsRemoval(const TNotNull<const UFaerieItemContainerBase*> Container,
	const ItemData::TNonNullViewPtr<Container::IAddressView> DataView, const FFaerieInventoryTag Reason) const
{
	// Tags that always deny removal.
	static FGameplayTagContainer RemovalDenyingTags = FGameplayTagContainer::CreateFromArray(
		TArray<FGameplayTag>{
			Inventory::Tags::CannotRemove
		});

	// Tags that deny a specific reason
	static TMap<FFaerieInventoryTag, FFaerieInventoryTag> OtherDenialTags = {
		{ Inventory::Tags::RemovalDeletion, Inventory::Tags::CannotDelete },
		{Inventory::Tags::RemovalMoving, Inventory::Tags::CannotMove },
		{ Inventory::Tags::RemovalEject, Inventory::Tags::CannotEject }
	};

	FGameplayTagContainer ThisEventTags = RemovalDenyingTags;

	// Check for a tag that might deny this reason
	if (auto&& DenialTag = OtherDenialTags.Find(Reason))
	{
		ThisEventTags.AddTag(*DenialTag);
	}

	if (const FConstStructView AddressData = GetDataForHandle(Container, DataView->ResolveAddress());
		AddressData.IsValid())
	{
		if (AddressData.Get<const FFaerieStorageEntryMetadata>().Tags.HasAny(ThisEventTags))
		{
			return EEventExtensionResponse::Disallowed;
		}
	}

	return EEventExtensionResponse::Allowed;
}

UScriptStruct* UInventoryMetadataExtension::GetDataScriptStruct() const
{
	return FFaerieStorageEntryMetadata::StaticStruct();
}

bool UInventoryMetadataExtension::DoesEntryHaveTag(const TNotNull<const UFaerieItemContainerBase*> Container,
	const FFaerieAddress Address, const FFaerieInventoryMetaTag Tag) const
{
	const FConstStructView AddressData = GetDataForHandle(Container, Address);
	if (!AddressData.IsValid())
	{
		return false;
	}

	return AddressData.Get<const FFaerieStorageEntryMetadata>().Tags.HasTag(Tag);
}

bool UInventoryMetadataExtension::CanSetEntryTag(const TNotNull<const UFaerieItemContainerBase*> Container,
	const FFaerieAddress Address, const FFaerieInventoryMetaTag Tag, const bool StateToSetTo) const
{
	return DoesEntryHaveTag(Container, Address, Tag) != StateToSetTo;
}

bool UInventoryMetadataExtension::MarkStackWithTag(const TNotNull<const UFaerieItemContainerBase*> Container,
	const FFaerieAddress Address, const FFaerieInventoryMetaTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetEntryTag(Container, Address, Tag, true))
	{
		return false;
	}

	return EditDataForHandle(Container, Address,
		[Tag](const FStructView Data)
		{
			Data.Get<FFaerieStorageEntryMetadata>().Tags.AddTag(Tag);
		});
}

void UInventoryMetadataExtension::TrySetTags(TNotNull<const UFaerieItemContainerBase*> Container,
	FFaerieAddress Address, const FGameplayTagContainer& Tags)
{
	EditDataForHandle(Container, Address,
		[Tags, this, Container, Address](const FStructView Data)
		{
			auto& Metadata = Data.Get<FFaerieStorageEntryMetadata>().Tags;

			for (auto&& Tag : Tags)
			{
				if (!Tag.IsValid())
				{
					continue;
				}

				const FFaerieInventoryMetaTag MetaTag = FFaerieInventoryMetaTag::ConvertChecked(Tag);

				if (!CanSetEntryTag(Container, Address, MetaTag, true))
				{
					continue;
				}

				Metadata.AddTag(MetaTag);
			}
		});
}

bool UInventoryMetadataExtension::ClearTagFromStack(const TNotNull<const UFaerieItemContainerBase*> Container,
	const FFaerieAddress Address, const FFaerieInventoryMetaTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetEntryTag(Container, Address, Tag, false))
	{
		return false;
	}

	return EditDataForHandle(Container, Address,
		[Tag](const FStructView Data)
		{
			Data.Get<FFaerieStorageEntryMetadata>().Tags.RemoveTag(Tag);
		});
}