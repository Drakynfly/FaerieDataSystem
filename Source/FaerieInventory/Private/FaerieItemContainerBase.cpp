// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "AssetLoadFlagFixer.h"
#include "FaerieInventoryLog.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Tokens/FaerieItemStorageToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerBase)

using namespace Faerie;

void FItemContainerFilter::ForEachMutable_NoBreak(TLoop<UFaerieItem*> Func) const
{
	Container->ForEachItem([&Func](const UFaerieItem* Item)
	{
		if (UFaerieItem* Mutable = Item->MutateCast())
		{
			Func(Mutable);
		}
	});
}

void FItemContainerFilter::ForEachMutable_WithBreak(TBreakableLoop<UFaerieItem*> Func) const
{
	// #@todo make ForEachItem breakable
	bool TempBreak = false;
	Container->ForEachItem([&Func, &TempBreak](const UFaerieItem* Item)
		{
			if (TempBreak) return;

			if (UFaerieItem* Mutable = Item->MutateCast())
			{
				if (Func(Mutable) == Stop)
				{
					TempBreak = true;
				}
			}
		});
}

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
	unimplemented();
	return FFaerieItemStack();
}

bool UFaerieItemContainerBase::Possess(FFaerieItemStack Stack)
{
	// This function should be implemented by children.
	unimplemented();
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
	Extensions->ForEachExtension(
		[this, &ExtensionData](const UItemContainerExtensionBase* Extension)
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
		});

	TSet<UFaerieItemContainerBase*> SubContainers;
	ForEachKey(
		[this, &SubContainers](const FEntryKey Key)
		{
			if (UFaerieItem* Item = View(Key).Item->MutateCast())
			{
				SubContainers.Append(UFaerieItemContainerToken::GetAllContainersInItem(Item));
			}
		});

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

	Extensions->ForEachExtension(
		[this](UItemContainerExtensionBase* Extension)
		{
			TryApplyUnclaimedSaveData(Extension);
		});

	TSet<UFaerieItemContainerBase*> SubContainers;
	Filter().ForEachMutable([&SubContainers](const UFaerieItem* Item)
		{
			if (UFaerieItem* Mutable = Item->MutateCast())
			{
				SubContainers.Append(UFaerieItemContainerToken::GetAllContainersInItem(Mutable));
			}
		});

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

Faerie::FItemContainerFilter UFaerieItemContainerBase::Filter() const
{
	return Faerie::FItemContainerFilter(this);
}
