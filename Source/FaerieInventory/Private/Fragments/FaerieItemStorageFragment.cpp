// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieItemStorageFragment.h"
#include "FaerieItemStorage.h"
#include "GameFramework/Actor.h"
#include "AssetLoadFlagFixer.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemOwnership.h"
#include "FaerieItemStackContainer.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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

bool FFaerieItemStorageFragment::InitializeRuntime(FMassEntityManager& EntityManager, const FMassEntityHandle Item)
{
	check(EntityManager.IsEntityValid(Item));
	if (IsValid(Storage.Storage))
	{
		UObject* OwnerObj = GetTransientPackageAsObject();

		// Only fully built items will already have an owner set.
		if (EntityManager.IsEntityBuilt(Item))
		{
			OwnerObj = Container::GetItemOwner(EntityManager, Item);
		}

		if (Utils::HasLoadFlag(Storage.Storage))
		{
			Storage.Storage = Utils::DuplicateObjectFromDiskForReplication(Storage.Storage.Get(), OwnerObj);
		}

		Storage.Storage->WriteContainerData(EntityManager, Container::FNestedContainer::StaticStruct(),
			[this, Item](const FStructView Element)
			{
				check(Item.IsValid())
				UE_LOGF(LogFaerieInventory, Log, "Assigned nested container item handle %ls to Storage '%ls'", *LexToString(Item), *GetNameSafe(Storage.Storage))
				Element.Get<Container::FNestedContainer>().ItemHandle = Item;
			}, true);
	}
	return true;
}

bool FFaerieItemStorageFragment::SaveDataConverter(FMassEntityManager& EntityManager, FInstancedStruct& SaveData)
{
	if (SaveData.IsValid())
	{
		// Loading
		Container::FLoadParams Params;

		if (!IsValid(Storage.Storage))
		{
			Storage.Storage = NewObject<UFaerieItemStorage>(GetTransientPackageAsObject());

			// We probably don't need to call this, because LoadSaveData also runs InitializeExtensions, so we can probably remove this
			Storage.Storage->InitializeContainer(EntityManager);
		}

		return Storage.Storage->LoadSaveData(EntityManager, SaveData, Params);
	}
	else
	{
		// Saving
		Container::FSaveParams Params;
		SaveData = Storage.Storage->MakeSaveData(EntityManager, Params);
		return SaveData.IsValid();
	}
}

FAERIE_REGISTER_TRAITS(FFaerieChildStackFragment)

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FFaerieChildStackFragmentValidation"

EDataValidationResult FFaerieChildStackFragment::IsDataValid(FDataValidationContext& Context) const
{
	for (auto&& Slot : Slots)
	{
		if (!IsValid(Slot.Stack))
		{
			Context.AddError(LOCTEXT("InvalidStack", "Stack invalid in ChildStackFragment. This fragment should not contain a null container!"));
			return EDataValidationResult::Invalid;
		}
	}
	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE

#endif

bool FFaerieChildStackFragment::InitializeRuntime(FMassEntityManager& EntityManager, const FMassEntityHandle Item)
{
	check(EntityManager.IsEntityValid(Item));
	UObject* OwnerObj = GetTransientPackageAsObject();

	// Only fully built items will already have an owner set.
	if (EntityManager.IsEntityBuilt(Item))
	{
		OwnerObj = Container::GetItemOwner(EntityManager, Item);
	}

	for (FFaerieInlineStackContainer& InlineStack : Slots)
	{
		if (InlineStack.Stack)
		{
			if (Utils::HasLoadFlag(InlineStack.Stack))
			{
				InlineStack.Stack = Utils::DuplicateObjectFromDiskForReplication(InlineStack.Stack.Get(), OwnerObj);
			}

			InlineStack.Stack->WriteContainerData(EntityManager, Container::FNestedContainer::StaticStruct(),
				[&, Item](const FStructView Element)
				{
					check(Item.IsValid())
					UE_LOGF(LogFaerieInventory, Log, "Assigned nested container item handle %ls to Stack '%ls'", *LexToString(Item), *GetNameSafe(InlineStack.Stack))
					Element.Get<Container::FNestedContainer>().ItemHandle = Item;
				}, true);
		}
	}
	return true;
}

bool FFaerieChildStackFragment::SaveDataConverter(FMassEntityManager& EntityManager, FInstancedStruct& SaveData)
{
	if (SaveData.IsValid())
	{
		// Loading
		if (SaveData.GetScriptStruct() != FFaerieChildStackFragmentSaveData::StaticStruct())
		{
			return false;
		}
		const FFaerieChildStackFragmentSaveData& SaveDataArray = SaveData.Get<FFaerieChildStackFragmentSaveData>();

		Container::FLoadParams Params;

		Slots.Empty(SaveDataArray.Stacks.Num());
		for (const FInstancedStruct& Stack : SaveDataArray.Stacks)
		{
			// @Todo do we need to pass the Instance in here as well...
			UFaerieItemStackContainer* NewStack = NewObject<UFaerieItemStackContainer>(GetTransientPackageAsObject());

			// We probably don't need to call this, because LoadSaveData also runs InitializeExtensions, so we can probably remove this
			NewStack->InitializeContainer(EntityManager);

			if (NewStack->LoadSaveData(EntityManager, Stack, Params))
			{
				Slots.Emplace(FFaerieInlineStackContainer(NewStack));
			}
		}
		return true;
	}
	else
	{
		// Saving
		Container::FSaveParams Params;

		SaveData.InitializeAs(FFaerieChildStackFragmentSaveData::StaticStruct());
		FFaerieChildStackFragmentSaveData& SaveDataArray = SaveData.GetMutable<FFaerieChildStackFragmentSaveData>();
		SaveDataArray.Stacks.Reserve(Slots.Num());

		for (const FFaerieInlineStackContainer& Stack : Slots)
		{
			FInstancedStruct StackSaveData = Stack.Stack->MakeSaveData(EntityManager, Params);
			if (!StackSaveData.IsValid())
			{
				continue;
			}
			SaveDataArray.Stacks.Emplace(StackSaveData);
		}

		return SaveData.IsValid();
	}
}
