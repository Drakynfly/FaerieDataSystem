﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieItemStorageToken.h"
#include "FaerieItemStorage.h"
#include "ItemContainerExtensionBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorageToken)

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

	TSet<UFaerieItemContainerBase*> Containers;

	Item->ForEachToken<UFaerieItemContainerToken>(
		[&Containers](const UFaerieItemContainerToken* Token)
		{
			Containers.Add(Token->ItemContainer);
			return true;
		});

	return Containers;
}

TSet<UFaerieItemContainerBase*> UFaerieItemContainerToken::GetContainersInItemOfClass(const UFaerieItem* Item,
	TSubclassOf<UFaerieItemContainerBase> Class)
{
	if (!ensure(IsValid(Item))) return {};

	TSet<UFaerieItemContainerBase*> Containers;

	Item->ForEachToken<UFaerieItemContainerToken>(
		[&Containers, Class](const UFaerieItemContainerToken* Token)
		{
			if (Token->ItemContainer->IsA(Class))
			{
				Containers.Add(Token->ItemContainer);
			}
			return true;
		});

	return Containers;
}

UFaerieItemStorageToken::UFaerieItemStorageToken()
{
	ItemContainer = CreateDefaultSubobject<UFaerieItemStorage>(FName{TEXTVIEW("ItemContainer")});
	Extensions = CreateDefaultSubobject<UItemContainerExtensionGroup>(FName{TEXTVIEW("Extensions")});
	ItemContainer->AddExtension(Extensions);
}

UFaerieItemStorage* UFaerieItemStorageToken::GetItemStorage() const
{
	return Cast<UFaerieItemStorage>(ItemContainer);
}