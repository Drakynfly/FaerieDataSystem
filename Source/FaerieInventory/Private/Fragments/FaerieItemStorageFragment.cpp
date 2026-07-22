// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieItemStorageFragment.h"
#include "FaerieItemStorage.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "AssetLoadFlagFixer.h"
#include "EntityManagerHelpers.h"
#include "FaerieItemStackContainer.h"

#include "Extensions/ItemContainerExtensionEvents.h"

#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorageFragment)

using namespace Faerie;

FAERIE_REGISTER_TRAITS(FFaerieItemStorageFragment)

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemStorageFragmentValidation"

EDataValidationResult FFaerieItemStorageFragment::IsDataValid(FDataValidationContext& Context) const
{
	if (!IsValid(Storage.Storage))
	{
		Context.AddError(LOCTEXT("InvalidStorage", "Storage invalid in ItemStorageFragment. This fragment should not contain a null container!"));
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE

#endif

bool FFaerieItemStorageFragment::InitializeRuntime(const TNotNull<UObject*> Outer, const ItemData::FMutableReference& Reference)
{
	if (IsValid(Storage.Storage))
	{
		Storage.Storage = Utils::DuplicateObjectFromDiskForReplication(Storage.Storage.Get(), Outer);

		if (UItemContainerExtensionEvents* Events = Extensions::Get<UItemContainerExtensionEvents>(Storage.Storage->GetExtensions(), true))
		{
			Events->GetOnPostEventBatch().AddStatic(&FFaerieItemStorageFragment::OnStorageItemChanged, Reference.GetInstance());
		}
	}
	return true;
}

void FFaerieItemStorageFragment::OnStorageItemChanged(const TNotNull<const UFaerieItemContainerBase*> Container, const Inventory::FEventLogBatch& EventLog, FFaerieItemInstance Instance)
{
	Instance.OnItemFragmentEdited(ItemData::FRequireEntityManager(Container), FFaerieItemStorageFragment::StaticStruct(), ItemData::Tags::FragmentGenericPropertyEdit);
}


FAERIE_REGISTER_TRAITS(FFaerieChildStackFragment)

bool FFaerieChildStackFragment::InitializeRuntime(const TNotNull<UObject*> Outer, const ItemData::FMutableReference& Reference)
{
	for (FFaerieInlineStackContainer& InlineStack : Slots)
	{
		if (InlineStack.Stack)
		{
			InlineStack.Stack = Utils::DuplicateObjectFromDiskForReplication(InlineStack.Stack.Get(), Outer);
			InlineStack.Stack->GetOnContainerEvent().AddStatic(&FFaerieChildStackFragment::OnSlotItemChanged, Reference.GetInstance());
		}
	}
	return true;
}

void FFaerieChildStackFragment::OnSlotItemChanged(const FFaerieItemProxy& Proxy, const FGameplayTag Tag,
	FFaerieItemInstance Instance)
{
	Instance.OnItemFragmentEdited(ItemData::FRequireEntityManager(Proxy.GetProxyObject()), FFaerieChildStackFragment::StaticStruct(), ItemData::Tags::FragmentGenericPropertyEdit);
}