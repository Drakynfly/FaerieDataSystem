﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "AssetLoadFlagFixer.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieInventoryLog.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Tokens/FaerieItemStorageToken.h"

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
	ensureAlwaysMsgf(!Faerie::HasLoadFlag(this),
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

void UFaerieItemContainerBase::OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, const FGameplayTag EditTag)
{
	// @todo more logic from the TakeOwnership protocol might belong here, in which case, maybe just move most of this there.
	if (EditTag == Tags::TokenAdd)
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
	if (EditTag == Tags::TokenRemove)
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
	UE_LOG(LogFaerieInventory, Log, TEXT("Adding Extension: '%s' to '%s'"), *Extension->GetFullName(), *this->GetFullName())
	if (Extensions->AddExtension(Extension))
	{
		TryApplyUnclaimedSaveData(Extension);
		return true;
	}
	return false;
}

void UFaerieItemContainerBase::RavelExtensionData(TMap<FGuid, FInstancedStruct>& ExtensionData) const
{
	for (auto Extension : FRecursiveConstExtensionIterator(Extensions))
	{
		const FGuid Identifier = Extension->GetIdentifier();
		if (!ensure(Identifier.IsValid())) return;

		// Skip if we have already included this extension.
		const uint32 IdentifierHash = GetTypeHash(Identifier);
		if (ExtensionData.ContainsByHash(IdentifierHash, Identifier)) return;

		if (const FInstancedStruct SaveData = Extension->MakeSaveData(this);
			SaveData.IsValid())
		{
			ExtensionData.AddByHash(IdentifierHash, Identifier, SaveData);
		}
	}

	TSet<UFaerieItemContainerBase*> SubContainers;
	for (UFaerieItem* Item : KeyFilter(this).Run<FMutableFilter>().Items())
	{
		SubContainers.Append(UFaerieItemContainerToken::GetAllContainersInItem(Item));
	}

	for (const UFaerieItemContainerBase* SubContainer : SubContainers)
	{
		SubContainer->RavelExtensionData(ExtensionData);
	}
}

void UFaerieItemContainerBase::UnravelExtensionData(UFaerieItemContainerExtensionData* ExtensionData)
{
	UnclaimedExtensionData = ExtensionData;
	if (!IsValid(UnclaimedExtensionData))
	{
		return;
	}

	for (auto&& Extension : FRecursiveExtensionIterator(Extensions))
	{
		TryApplyUnclaimedSaveData(Extension);
	}

	TSet<UFaerieItemContainerBase*> SubContainers;
	for (UFaerieItem* Item : KeyFilter(this).Run<FMutableFilter>().Items())
	{
		SubContainers.Append(UFaerieItemContainerToken::GetAllContainersInItem(Item));
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

// Note: Implementations for these PURE_VIRTUALS need to be here because TUniquePtr complains about their dtors if they are forward declared.
TUniquePtr<IContainerIterator> UFaerieItemContainerBase::CreateIterator() const
PURE_VIRTUAL(UFaerieItemContainerBase::CreateIterator, return TUniquePtr<Faerie::IContainerIterator>(); )

TUniquePtr<IContainerFilter> UFaerieItemContainerBase::CreateFilter(bool FilterByAddresses) const
PURE_VIRTUAL(UFaerieItemContainerBase::CreateFilter, return TUniquePtr<Faerie::IContainerFilter>(); )
