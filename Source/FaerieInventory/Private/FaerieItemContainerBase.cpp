// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerBase.h"
#include "FaerieInventorySettings.h"

#include "FaerieItemStorage.h"
#include "FaerieUtils.h"
#include "ItemContainerExtensionBase.h"
#include "Net/UnrealNetwork.h"
#include "Tokens/FaerieItemStorageToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerBase)

UFaerieItemContainerBase::UFaerieItemContainerBase()
{
	Extensions = CreateDefaultSubobject<UItemContainerExtensionGroup>(FName{TEXTVIEW("Extensions")});
	Extensions->SetIdentifier();
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

UItemContainerExtensionGroup* UFaerieItemContainerBase::GetExtensionGroup() const
{
	return Extensions;
}

bool UFaerieItemContainerBase::AddExtension(UItemContainerExtensionBase* Extension)
{
	UE_LOG(LogTemp, Log, TEXT("Adding Extension: '%s' to '%s'"), *Extension->GetFullName(), *this->GetFullName())
	if (Extensions->AddExtension(Extension))
	{
		TryApplyUnclaimedSaveData(Extension);
		return true;
	}
	return false;
}

void UFaerieItemContainerBase::RavelExtensionData(TMap<FGuid, FInstancedStruct>& Data) const
{
	Extensions->ForEachExtension(
		[this, &Data](const UItemContainerExtensionBase* Extension)
		{
			const FGuid Identifier = Extension->GetIdentifier();
			if (!ensure(Identifier.IsValid())) return;

			if (const FInstancedStruct SaveData = Extension->MakeSaveData(this);
				SaveData.IsValid())
			{
				Data.Add(Identifier, SaveData);
			}
		});

	TSet<UFaerieItemContainerBase*> SubContainers;
	ForEachKey(
		[this, &SubContainers](const FEntryKey Key)
		{
			if (const UFaerieItem* Item = View(Key).Item.Get())
			{
				SubContainers.Append(UFaerieItemContainerToken::GetAllContainersInItem(Item));
			}
		});

	for (const UFaerieItemContainerBase* SubContainer : SubContainers)
	{
		SubContainer->RavelExtensionData(Data);
	}
}

void UFaerieItemContainerBase::UnravelExtensionData(const TMap<FGuid, FInstancedStruct>& Data)
{
	UnclaimedExtensionData = Data;

	Extensions->ForEachExtension(
		[this](UItemContainerExtensionBase* Extension)
		{
			TryApplyUnclaimedSaveData(Extension);
		});

	TSet<UFaerieItemContainerBase*> SubContainers;
	ForEachKey(
		[this, &SubContainers](const FEntryKey Key)
		{
			if (const UFaerieItem* Item = View(Key).Item.Get())
			{
				SubContainers.Append(UFaerieItemContainerToken::GetAllContainersInItem(Item));
			}
		});

	for (UFaerieItemContainerBase* SubContainer : SubContainers)
	{
		SubContainer->UnravelExtensionData(UnclaimedExtensionData);
	}
}

void UFaerieItemContainerBase::TryApplyUnclaimedSaveData(UItemContainerExtensionBase* Extension)
{
	const FGuid Identifier = Extension->Identifier;
	if (!ensure(Identifier.IsValid())) return;

	const uint32 IdentifierHash = GetTypeHash(Identifier);
	if (auto&& SaveData = UnclaimedExtensionData.FindByHash(IdentifierHash, Identifier))
	{
		Extension->LoadSaveData(this, *SaveData);
		UnclaimedExtensionData.RemoveByHash(IdentifierHash, Identifier);
	}
}

void UFaerieItemContainerBase::OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, const FGameplayTag EditTag)
{
	if (EditTag == Faerie::Tags::TokenAdd)
	{
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			auto MutableToken = const_cast<UFaerieItemToken*>(Token);
			Actor->AddReplicatedSubObject(MutableToken);
			MutableToken->InitializeNetObject(Actor);
		}
		return;
	}
	if (EditTag == Faerie::Tags::TokenRemove)
	{
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			auto MutableToken = const_cast<UFaerieItemToken*>(Token);
			Actor->RemoveReplicatedSubObject(MutableToken);
			MutableToken->DeinitializeNetObject(Actor);
		}
		return;
	}
}

void UFaerieItemContainerBase::ReleaseOwnership(const UFaerieItem* Item)
{
	if (!ensure(IsValid(Item))) return;

	// When Items are potentially mutable, undo any modifications that rely on this owner.
	if (const auto MutableItem = Item->MutateCast())
	{
		// @todo this logic could be moved to UFaerieItem::PostRename (if we enforce the RenameBehavior)
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			MutableItem->DeinitializeNetObject(Actor);
			Actor->RemoveReplicatedSubObject(MutableItem);
			MutableItem->ForEachToken(
				[Actor](const TObjectPtr<UFaerieItemToken>& Token)
				{
					Token->DeinitializeNetObject(Actor);
					Actor->RemoveReplicatedSubObject(Token);
					return true;
				});
		}

		// If we renamed the item to ourself when we took ownership of this item, then we need to release that now.
		if (MutableItem->GetOuter() == this)
		{
			MutableItem->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors);
		}

		MutableItem->GetNotifyOwnerOfSelfMutation().Unbind();

		// Remove our group of extensions from any sub-storages
		for (auto&& ChildContainers = UFaerieItemContainerToken::GetAllContainersInItem(MutableItem);
			 auto&& ChildContainer : ChildContainers)
		{
			ChildContainer->RemoveExtension(Extensions);
		}
	}
}

void UFaerieItemContainerBase::TakeOwnership(const UFaerieItem* Item)
{
	if (!ensure(IsValid(Item))) return;

	if (UFaerieItem* MutableItem = Item->MutateCast())
	{
		checkfSlow(Item->GetOuter() == GetTransientPackage(), TEXT("ReleaseOwnership was not called correctly on this item, before attempting to give ownership here!"));
		checkfSlow(!MutableItem->GetNotifyOwnerOfSelfMutation().IsBound(), TEXT("This should always have been unbound by the previous owner!"))
		MutableItem->GetNotifyOwnerOfSelfMutation().BindUObject(this, &ThisClass::OnItemMutated);

		if (GetDefault<UFaerieInventorySettings>()->ContainerMutableBehavior == EFaerieContainerOwnershipBehavior::Rename)
		{
			MutableItem->Rename(nullptr, this, REN_DontCreateRedirectors);
		}

		// @todo this logic could be moved to UFaerieItem::PostRename (if we enforce the RenameBehavior)
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			Actor->AddReplicatedSubObject(MutableItem);
			MutableItem->InitializeNetObject(Actor);
			MutableItem->ForEachToken(
				[Actor](const TObjectPtr<UFaerieItemToken>& Token)
				{
					Actor->AddReplicatedSubObject(Token);
					Token->InitializeNetObject(Actor);
					return true;
				});
		}

		// Add our group of extensions to any sub-storages
		for (auto&& ChildContainers = UFaerieItemContainerToken::GetAllContainersInItem(MutableItem);
			 auto&& Container : ChildContainers)
		{
			Container->AddExtension(Extensions);
		}
	}
}