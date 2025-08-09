// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataStackLiteral.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataStackLiteral)

const UFaerieItem* UFaerieItemDataStackLiteral::GetItemObject() const
{
	return ItemStack.Item;
}

int32 UFaerieItemDataStackLiteral::GetCopies() const
{
	return ItemStack.Copies;
}

FFaerieItemStack UFaerieItemDataStackLiteral::Release(const FFaerieItemStackView Stack)
{
	FFaerieItemStack OutStack;

	if (ItemStack.Item == Stack.Item &&
		ItemStack.Copies >= Stack.Copies)
	{
		ItemStack.Copies -= Stack.Copies;
		OutStack.Item = ItemStack.Item;
		OutStack.Copies = Stack.Copies;

		if (ItemStack.Copies <= 0)
		{
			ItemStack.Item = nullptr;
		}
	}

	return OutStack;
}

bool UFaerieItemDataStackLiteral::Possess(const FFaerieItemStack Stack)
{
	// @todo this is wonky behavior
	if (ItemStack.Item == Stack.Item)
	{
		ItemStack.Copies += Stack.Copies;
	}
	else
	{
		ItemStack = Stack;
	}
	return true;
}

void UFaerieItemDataStackLiteral::SetValue(const UFaerieItem* Item)
{
	ItemStack = FFaerieItemStack(Item, 1);
}

void UFaerieItemDataStackLiteral::SetValue(const FFaerieItemStack InStack)
{
	ItemStack = InStack;
}

UFaerieItemDataStackLiteral* UFaerieItemDataStackLiteral::CreateItemDataStackLiteral(const FFaerieItemStack InStack)
{
	UFaerieItemDataStackLiteral* Literal = NewObject<UFaerieItemDataStackLiteral>();
	Literal->SetValue(InStack);
	return Literal;
}