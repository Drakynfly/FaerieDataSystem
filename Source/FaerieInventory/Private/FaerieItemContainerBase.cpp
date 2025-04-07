// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerBase.h"
#include "FaerieInventorySettings.h"

#include "FaerieItemStorage.h"
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

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Extensions, Params);
}

void UFaerieItemContainerBase::InitializeNetObject(AActor* Actor)
{
	Extensions->InitializeExtension(this);
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
			Actor->AddReplicatedSubObject(const_cast<UFaerieItemToken*>(Token));
		}
		return;
	}
	if (EditTag == Faerie::Tags::TokenRemove)
	{
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			Actor->RemoveReplicatedSubObject(const_cast<UFaerieItemToken*>(Token));
		}
		return;
	}
}

void UFaerieItemContainerBase::ReleaseOwnership(UFaerieItem* Item)
{
	if (!ensure(IsValid(Item))) return;

	// When Items are potentially mutable, undo any modifications that rely on this owner.
	if (Item->IsInstanceMutable())
	{
		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			Actor->RemoveReplicatedSubObject(Item);
			Item->ForEachToken(
				[Actor](const TObjectPtr<UFaerieItemToken>& Token)
				{
					Actor->RemoveReplicatedSubObject(ConstCast(Token));
					return true;
				});
		}

		// If we renamed the item to ourself when we took ownership of this item, then we need to release that now.
		if (Item->GetOuter() == this)
		{
			Item->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors);
		}

		Item->GetNotifyOwnerOfSelfMutation().Unbind();

		// Remove our group of extensions from any sub-storages
		for (auto&& ChildContainers = UFaerieItemContainerToken::GetAllContainersInItem(Item);
			 auto&& ChildContainer : ChildContainers)
		{
			ChildContainer->RemoveExtension(Extensions);
		}
	}
}

void UFaerieItemContainerBase::TakeOwnership(UFaerieItem* Item)
{
	if (!ensure(IsValid(Item))) return;

	if (Item->IsInstanceMutable())
	{
		checkfSlow(Item->GetOuter() == GetTransientPackage(), TEXT("ReleaseOwnership was not called correctly on this item, before attempting to give ownership here!"));
		checkfSlow(!Item->GetNotifyOwnerOfSelfMutation().IsBound(), TEXT("This should always have been unbound by the previous owner!"))
		Item->GetNotifyOwnerOfSelfMutation().BindUObject(this, &ThisClass::OnItemMutated);

		if (GetDefault<UFaerieInventorySettings>()->ContainerMutableBehavior == EFaerieContainerOwnershipBehavior::Rename)
		{
			Item->Rename(nullptr, this, REN_DontCreateRedirectors);
		}

		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			Actor->AddReplicatedSubObject(Item);
			Item->ForEachToken(
				[Actor](const TObjectPtr<UFaerieItemToken>& Token)
				{
					Actor->AddReplicatedSubObject(ConstCast(Token));
					return true;
				});
		}

		// Add our group of extensions to any sub-storages
		for (auto&& ChildContainers = UFaerieItemContainerToken::GetAllContainersInItem(Item);
			 auto&& Container : ChildContainers)
		{
			Container->AddExtension(Extensions);
		}
	}
}