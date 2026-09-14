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

bool FFaerieItemStorageFragment::InitializeRuntime(FMassEntityManager& EntityManager, const FFaerieItemInstance& Instance)
{
	if (IsValid(Storage.Storage))
	{
		UObject* OwnerObj = Container::GetItemOwner(EntityManager, Instance);
		if (!OwnerObj)
		{
			// @todo handle these
			UE_LOGF(LogFaerieInventory, Warning, "Unable to retrieve owner from instance. This instance must be reparented when possessed!")
			OwnerObj = GetTransientPackageAsObject();
		}

		Storage.Storage = Utils::DuplicateObjectFromDiskForReplication(Storage.Storage.Get(), OwnerObj);
		Storage.Storage->WriteContainerData(Container::FNestedContainer::StaticStruct(),
			[Instance](const FStructView Element)
			{
				Element.Get<Container::FNestedContainer>().ItemHandle = Instance.GetMassEntityHandle();
			});
	}
	return true;
}

FAERIE_REGISTER_TRAITS(FFaerieChildStackFragment)

bool FFaerieChildStackFragment::InitializeRuntime(FMassEntityManager& EntityManager, const FFaerieItemInstance& Instance)
{
	UObject* OwnerObj = Container::GetItemOwner(EntityManager, Instance);
	if (!OwnerObj)
	{
		// @todo handle these
		UE_LOGF(LogFaerieInventory, Warning, "Unable to retrieve owner from instance. This instance must be reparented when possessed!")
		OwnerObj = GetTransientPackageAsObject();
	}

	for (FFaerieInlineStackContainer& InlineStack : Slots)
	{
		if (InlineStack.Stack)
		{
			InlineStack.Stack = Utils::DuplicateObjectFromDiskForReplication(InlineStack.Stack.Get(), OwnerObj);
			InlineStack.Stack->WriteContainerData(Container::FNestedContainer::StaticStruct(),
				[Instance](const FStructView Element)
				{
					Element.Get<Container::FNestedContainer>().ItemHandle = Instance.GetMassEntityHandle();
				});
		}
	}
	return true;
}