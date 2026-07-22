// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerBase.h"
#include "AssetLoadFlagFixer.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerFilter.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
#include "FaerieSubObjectFilter.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerBase)

using namespace Faerie;

UFaerieItemContainerBase::UFaerieItemContainerBase()
{
	Extensions = CreateDefaultSubobject<UItemContainerExtensionGroup>(FName{TEXTVIEW("Extensions")});
	SET_NEW_IDENTIFIER(Extensions, TEXTVIEW("ItemContainerBaseGroup"))
}

void UFaerieItemContainerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//FDoRepLifetimeParams Params;
	//Params.bIsPushBased = true;
	DOREPLIFETIME_CONDITION(ThisClass, Extensions, COND_InitialOnly);
}

void UFaerieItemContainerBase::InitializeNetObject(const TNotNull<AActor*> Actor)
{
	ensureAlwaysMsgf(!Faerie::Utils::HasLoadFlag(this),
		TEXT("Containers must not be assets loaded from disk. (DuplicateObjectFromDiskForReplication or ClearLoadFlags can fix this)"
			LINE_TERMINATOR
			"	Failing Container: '%s'"), *GetFullName());
	Actor->AddReplicatedSubObject(Extensions);
	Extensions->InitializeNetObject(Actor);
	Extensions::FGroupAPI::InitializeExtension(Extensions, this);
}

void UFaerieItemContainerBase::DeinitializeNetObject(const TNotNull<AActor*> Actor)
{
	Actor->RemoveReplicatedSubObject(Extensions);
	Extensions->DeinitializeNetObject(Actor);
	Extensions::FGroupAPI::DeinitializeExtension(Extensions, this);
}

void UFaerieItemContainerBase::DestroyStack(const FFaerieItemProxy& Proxy, int32 Copies)
{
	// This function should be implemented by children.
	checkNoEntry();
}

bool UFaerieItemContainerBase::Possess(const FFaerieUnownedItemStack& Stack)
{
	// This function should be implemented by children.
	checkNoEntry();
	return false;
}

void UFaerieItemContainerBase::OnItemDataChanged(const ItemData::FMutableReference& Instance, const TNotNull<const UScriptStruct*> Struct, const FGameplayTag EditTag)
{
}

UItemContainerExtensionGroup* UFaerieItemContainerBase::VirtualGetExtensionGroup() const
{
	return Extensions;
}

FFaerieItemExportData UFaerieItemContainerBase::ExportItemData(const ItemData::FRequireEntityManager& EntityManager, const ItemData::FReference& Item) const
{
	static constexpr ItemData::EMassFragmentExportOptions ExportOptions = ItemData::EMassFragmentExportOptions::OnlyFaerieMassFragments;

	FFaerieItemExportData ExportData;
	Item.GetInstance().ExportFragmentData(EntityManager, ExportData.MassInstances, ExportOptions);
	return ExportData;
}

FFaerieItemInstance UFaerieItemContainerBase::ImportItemData(const ItemData::FRequireEntityManager& EntityManager, const UFaerieItem* Item,
	const FFaerieItemExportData& ExportData)
{
	FFaerieItemInstance Instance = FFaerieItemInstance::FromPointer(Item);
	TArray<FInstancedStruct> FragmentCopy = ExportData.MassInstances;
	Instance.ImportFragmentData(EntityManager, FragmentCopy);
	return Instance;
}

void UFaerieItemContainerBase::RavelExtensionData(FFaerieItemContainerExtensionData& ExtensionData) const
{
	using namespace Faerie;

	auto ExtractSaveData = [&ExtensionData](const UFaerieItemContainerBase* Container)
	{
		const uint32 ContainerHash = GetTypeHash(Container->GetName());

		for (auto&& Extension : Extensions::FRecursiveConstExtensionIterator(Container->Extensions))
		{
			const FGuid Identifier = Extension->GetIdentifier();
			if (!ensure(Identifier.IsValid())) return;

			const uint32 ExtensionHash = GetTypeHash(Identifier);

			// Unique hash for the combo of this extension + container.
			const uint32 SaveHash = HashCombine(ExtensionHash, ContainerHash);

			// Skip if we have already included this extension.
			if (ExtensionData.Data.Contains(SaveHash)) return;
			if (const FInstancedStruct SaveData = Extension->MakeSaveData(Container);
				SaveData.IsValid())
			{
				ExtensionData.Data.Add(SaveHash, SaveData);
			}
		}
	};

	ExtractSaveData(this);

	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	for (auto It = Container::MutableItemRange(this); It; ++It)
	{
		for (const UFaerieItemContainerBase* Container : SubObject::IterateRecursive(EntityManager, *It))
		{
			ExtractSaveData(Container);
		}
	}
}

void UFaerieItemContainerBase::UnravelExtensionData(const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData)
{
	using namespace Faerie;

	for (auto&& Extension : Extensions::FRecursiveExtensionIterator(Extensions))
	{
		Extensions->TryApplyUnclaimedSaveData(Extension);
	}

	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	TArray<TNotNull<UFaerieItemContainerBase*>> SubContainers;
	for (auto It = Container::MutableItemRange(this); It; ++It)
	{
		SubObject::GetContainersInInstanceDirect(EntityManager, *It, SubContainers);
	}

	for (const TNotNull<UFaerieItemContainerBase*> SubContainer : SubContainers)
	{
		SubContainer->UnravelExtensionData(ExtensionData);
	}

	if (ExtensionData.IsValid() && !ExtensionData.Get().Data.IsEmpty())
	{
		Extensions->SetUnclaimedExtensionData(ExtensionData);
	}
}

// Note: Implementations for these PURE_VIRTUAL need to be here because TUniquePtr complains about their dtors if they are forward declared.
TUniquePtr<Container::IEntryIterator> UFaerieItemContainerBase::CreateEntryIterator() const
	PURE_VIRTUAL(UFaerieItemContainerBase::CreateIterator, return nullptr; )

TUniquePtr<Container::IAddressIterator> UFaerieItemContainerBase::CreateAddressIterator() const
	PURE_VIRTUAL(UFaerieItemContainerBase::CreateIterator, return nullptr; )

TUniquePtr<Container::IAddressIterator> UFaerieItemContainerBase::CreateSingleEntryIterator(FFaerieEntryKey Key) const
	PURE_VIRTUAL(UFaerieItemContainerBase::CreateIterator, return nullptr; )

