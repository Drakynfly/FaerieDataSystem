// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieItemStorageToken.h"
#include "FaerieItemStorage.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorageToken)

using namespace Faerie;

void UFaerieItemContainerToken::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ItemContainer, COND_InitialOnly);
}

bool UFaerieItemContainerToken::IsMutable() const
{
	// Container tokens always make their owner mutable, as should be obvious. If an item can contain arbitrary content
	// then we have no way to determine its mutability state.
	return true;
}

TSet<UFaerieItemContainerBase*> UFaerieItemContainerToken::GetAllContainersInItem(const UFaerieItem* Item)
{
	if (!ensure(IsValid(Item))) return {};

	// @note: Technically not respecting const-purity, but all Containers imply mutable anyway, so it doesn't matter.

	TSet<UFaerieItemContainerBase*> Containers;

	Item->ForEachToken<UFaerieItemContainerToken>(
		[&Containers](const TObjectPtr<const UFaerieItemContainerToken>& Token)
		{
			Containers.Add(Token->ItemContainer);
			return Continue;
		});

	return Containers;
}

TSet<UFaerieItemContainerBase*> UFaerieItemContainerToken::GetContainersInItemOfClass(const UFaerieItem* Item,
	TSubclassOf<UFaerieItemContainerBase> Class)
{
	if (!ensure(IsValid(Item))) return {};

	// @note: Technically not respecting const-purity, but all Containers imply mutable anyway, so it doesn't matter.

	TSet<UFaerieItemContainerBase*> Containers;

	Item->ForEachToken<UFaerieItemContainerToken>(
		[&Containers, Class](const TObjectPtr<const UFaerieItemContainerToken>& Token)
		{
			if (Token->ItemContainer->IsA(Class))
			{
				Containers.Add(Token->ItemContainer);
			}
			return Continue;
		});

	return Containers;
}

UFaerieItemStorageToken::UFaerieItemStorageToken()
{
	ItemContainer = CreateDefaultSubobject<UFaerieItemStorage>(FName{TEXTVIEW("ItemContainer")});
	Extensions = CreateDefaultSubobject<UItemContainerExtensionGroup>(FName{TEXTVIEW("Extensions")});
	SET_NEW_IDENTIFIER(Extensions, TEXT("ItemStorageTokenGroup"))
}

void UFaerieItemStorageToken::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, Extensions, COND_InitialOnly);
}

void UFaerieItemStorageToken::InitializeNetObject(AActor* Actor)
{
	Actor->AddReplicatedSubObject(ItemContainer);
	ItemContainer->InitializeNetObject(Actor);

	Extensions->ReplicationFixup();
	Actor->AddReplicatedSubObject(Extensions);
	Extensions->InitializeNetObject(Actor);

	ItemContainer->GetExtensionGroup()->SetParentGroup(Extensions);
}

void UFaerieItemStorageToken::DeinitializeNetObject(AActor* Actor)
{
	ItemContainer->GetExtensionGroup()->SetParentGroup(nullptr);

	Actor->RemoveReplicatedSubObject(ItemContainer);
	ItemContainer->DeinitializeNetObject(Actor);

	Actor->RemoveReplicatedSubObject(Extensions);
	Extensions->DeinitializeNetObject(Actor);
}

UFaerieItemStorage* UFaerieItemStorageToken::GetItemStorage() const
{
	return Cast<UFaerieItemStorage>(ItemContainer);
}
