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

EEventExtensionResponse UInventoryMetadataExtension::AllowsRemoval(const UFaerieItemContainerBase* Container,
	const FFaerieAddress Address, const FFaerieInventoryTag Reason) const
{
	// Tags that always deny removal.
	static FGameplayTagContainer RemovalDenyingTags = FGameplayTagContainer::CreateFromArray(
		TArray<FGameplayTag>{
			Faerie::Inventory::Tags::CannotRemove
		});

	// Tags that deny a specific reason
	static TMap<FFaerieInventoryTag, FFaerieInventoryTag> OtherDenialTags = {
		{ Faerie::Inventory::Tags::RemovalDeletion, Faerie::Inventory::Tags::CannotDelete },
		{Faerie::Inventory::Tags::RemovalMoving, Faerie::Inventory::Tags::CannotMove },
		{ Faerie::Inventory::Tags::RemovalEject, Faerie::Inventory::Tags::CannotEject }
	};

	FGameplayTagContainer ThisEventTags = RemovalDenyingTags;

	// Check for a tag that might deny this reason
	if (auto&& DenialTag = OtherDenialTags.Find(Reason))
	{
		ThisEventTags.AddTag(*DenialTag);
	}

	FFaerieAddressableHandle Handle;
	Handle.Container = ConstCast(ObjectPtrWrap(Container));
	Handle.Address = Address;

	if (const FConstStructView DataView = GetDataForHandle(Handle);
		DataView.IsValid())
	{
		if (DataView.Get<const FInventoryEntryMetadata>().Tags.HasAny(ThisEventTags))
		{
			return EEventExtensionResponse::Disallowed;
		}
	}

	return EEventExtensionResponse::Allowed;
}

UScriptStruct* UInventoryMetadataExtension::GetDataScriptStruct() const
{
	return FInventoryEntryMetadata::StaticStruct();
}

bool UInventoryMetadataExtension::DoesEntryHaveTag(const FFaerieAddressableHandle Handle, const FFaerieInventoryMetaTag Tag) const
{
	const FConstStructView DataView = GetDataForHandle(Handle);
	if (!DataView.IsValid())
	{
		return false;
	}

	return DataView.Get<const FInventoryEntryMetadata>().Tags.HasTag(Tag);
}

bool UInventoryMetadataExtension::CanSetEntryTag(const FFaerieAddressableHandle Handle,
												 const FFaerieInventoryMetaTag Tag, const bool StateToSetTo) const
{
	return DoesEntryHaveTag(Handle, Tag) != StateToSetTo;
}

bool UInventoryMetadataExtension::MarkStackWithTag(const FFaerieAddressableHandle Handle, const FFaerieInventoryMetaTag Tag)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetEntryTag(Handle, Tag, true))
	{
		return false;
	}

	return EditDataForHandle(Handle,
		[Tag](const FStructView Data)
		{
			Data.Get<FInventoryEntryMetadata>().Tags.AddTag(Tag);
		});
}

void UInventoryMetadataExtension::TrySetTags(const FFaerieAddressableHandle Handle, const FGameplayTagContainer& Tags)
{
	if (!Handle.IsValid())
	{
		return;
	}

	EditDataForHandle(Handle,
		[Tags, this, Handle](const FStructView Data)
		{
			auto& Metadata = Data.Get<FInventoryEntryMetadata>().Tags;

			for (auto&& Tag : Tags)
			{
				if (!Tag.IsValid())
				{
					continue;
				}

				const FFaerieInventoryMetaTag MetaTag = FFaerieInventoryMetaTag::ConvertChecked(Tag);

				if (!CanSetEntryTag(Handle, MetaTag, true))
				{
					continue;
				}

				Metadata.AddTag(MetaTag);
			}
		});
}

bool UInventoryMetadataExtension::ClearTagFromStack(const FFaerieAddressableHandle Handle, const FFaerieInventoryMetaTag Tag)
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!CanSetEntryTag(Handle, Tag, false))
	{
		return false;
	}

	return EditDataForHandle(Handle,
		[Tag](const FStructView Data)
		{
			Data.Get<FInventoryEntryMetadata>().Tags.RemoveTag(Tag);
		});
}