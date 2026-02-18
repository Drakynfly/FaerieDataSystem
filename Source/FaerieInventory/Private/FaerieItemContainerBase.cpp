// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "AssetLoadFlagFixer.h"
#include "FaerieContainerFilter.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemToken.h"
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

void UFaerieItemContainerBase::InitializeNetObject(AActor* Actor)
{
	ensureAlwaysMsgf(!Faerie::Utils::HasLoadFlag(this),
		TEXT("Containers must not be assets loaded from disk. (DuplicateObjectFromDiskForReplication or ClearLoadFlags can fix this)"
			LINE_TERMINATOR
			"	Failing Container: '%s'"), *GetFullName());
	Actor->AddReplicatedSubObject(Extensions);
	Extensions->InitializeNetObject(Actor);
	Extensions->InitializeExtension(this);
}

void UFaerieItemContainerBase::DeinitializeNetObject(AActor* Actor)
{
	Actor->RemoveReplicatedSubObject(Extensions);
	Extensions->DeinitializeNetObject(Actor);
	Extensions->DeinitializeExtension(this);
}

FFaerieItemStack UFaerieItemContainerBase::Release(const FFaerieItemStackView Stack)
{
	// This function should be implemented by children.
	checkNoEntry();
	return FFaerieItemStack();
}

bool UFaerieItemContainerBase::Possess(FFaerieItemStack Stack)
{
	// This function should be implemented by children.
	checkNoEntry();
	return false;
}

void UFaerieItemContainerBase::OnItemMutated(const TNotNull<const UFaerieItem*> Item, const TNotNull<const UFaerieItemToken*> Token, const FGameplayTag EditTag)
{
	// @todo more logic from the TakeOwnership protocol might belong here, in which case, maybe just move most of this there.
	if (EditTag == Token::Tags::TokenAdd)
	{
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			if (UFaerieItemToken* MutableToken = Token->MutateCast())
			{
				Actor->AddReplicatedSubObject(MutableToken);
				MutableToken->InitializeNetObject(Actor);
			}
		}
		return;
	}
	if (EditTag == Token::Tags::TokenRemove)
	{
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			if (UFaerieItemToken* MutableToken = Token->MutateCast())
			{
				Actor->RemoveReplicatedSubObject(MutableToken);
				MutableToken->DeinitializeNetObject(Actor);
			}
		}
		return;
	}
}

UItemContainerExtensionGroup* UFaerieItemContainerBase::GetExtensionGroup() const
{
	return Extensions;
}

bool UFaerieItemContainerBase::AddExtension(UItemContainerExtensionBase* Extension)
{
	UE_LOG(LogFaerieInventory, Verbose, TEXT("Adding Extension: '%s' to '%s'"), *Extension->GetFullName(), *this->GetFullName())
	if (Extensions->AddExtension(Extension))
	{
		TryApplyUnclaimedSaveData(Extension);
		return true;
	}
	return false;
}

void UFaerieItemContainerBase::RavelExtensionData(TMap<FGuid, FInstancedStruct>& ExtensionData) const
{
	using namespace Faerie;

	auto ExtractSaveData = [&ExtensionData](const UFaerieItemContainerBase* Container)
	{
		for (auto&& Extension : Extensions::FRecursiveConstExtensionIterator(Container->Extensions))
		{
			const FGuid Identifier = Extension->GetIdentifier();
			if (!ensure(Identifier.IsValid())) return;

			// Skip if we have already included this extension.
			const uint32 IdentifierHash = GetTypeHash(Identifier);
			if (ExtensionData.ContainsByHash(IdentifierHash, Identifier)) return;

			if (const FInstancedStruct SaveData = Extension->MakeSaveData(Container);
				SaveData.IsValid())
			{
				ExtensionData.AddByHash(IdentifierHash, Identifier, SaveData);
			}
		}
	};

	ExtractSaveData(this);

	for (UFaerieItem* Item : Container::ItemRange(this))
	{
		for (const UFaerieItemContainerBase* Container : SubObject::IterateRecursive(Item))
		{
			ExtractSaveData(Container);
		}
	}
}

void UFaerieItemContainerBase::UnravelExtensionData(UFaerieItemContainerExtensionData* ExtensionData)
{
	using namespace Faerie;

	UnclaimedExtensionData = ExtensionData;
	if (!IsValid(UnclaimedExtensionData))
	{
		return;
	}

	for (auto&& Extension : Extensions::FRecursiveExtensionIterator(Extensions))
	{
		TryApplyUnclaimedSaveData(Extension);
	}

	TArray<UFaerieItemContainerBase*> SubContainers;
	for (UFaerieItem* Item : Container::ItemRange(this))
	{
		SubContainers.Append(SubObject::GetAllContainersInItem(Item));
	}

	for (UFaerieItemContainerBase* SubContainer : SubContainers)
	{
		SubContainer->UnravelExtensionData(UnclaimedExtensionData);
	}
}

void UFaerieItemContainerBase::TryApplyUnclaimedSaveData(UItemContainerExtensionBase* Extension)
{
	if (!IsValid(UnclaimedExtensionData))
	{
		return;
	}

	const FGuid Identifier = Extension->Identifier;
	if (!ensure(Identifier.IsValid())) return;

	const uint32 IdentifierHash = GetTypeHash(Identifier);
	if (auto&& SaveData = UnclaimedExtensionData->Data.FindByHash(IdentifierHash, Identifier))
	{
		Extension->LoadSaveData(this, *SaveData);
		UnclaimedExtensionData->Data.RemoveByHash(IdentifierHash, Identifier);
	}
}

// Note: Implementations for these PURE_VIRTUAL need to be here because TUniquePtr complains about their dtors if they are forward declared.
TUniquePtr<Container::IIterator> UFaerieItemContainerBase::CreateEntryIterator() const
PURE_VIRTUAL(UFaerieItemContainerBase::CreateIterator, return nullptr; )

TUniquePtr<Container::IIterator> UFaerieItemContainerBase::CreateAddressIterator() const
PURE_VIRTUAL(UFaerieItemContainerBase::CreateIterator, return nullptr; )

TUniquePtr<Container::IIterator> UFaerieItemContainerBase::CreateSingleEntryIterator(FEntryKey Key) const
PURE_VIRTUAL(UFaerieItemContainerBase::CreateIterator, return nullptr; )