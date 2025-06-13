// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieCardTokenBase.h"

#include "FaerieItem.h"
#include "FaerieItemDataProxy.h"
#include "FaerieItemToken.h"
#include "Widgets/FaerieCardBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardTokenBase)

void UFaerieCardTokenBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (auto&& OuterCard = GetTypedOuter<UFaerieCardBase>())
	{
		OuterCard->GetOnCardRefreshed().AddUObject(this, &ThisClass::OnCardRefreshed);
	}
}

void UFaerieCardTokenBase::NativeDestruct()
{
	if (auto&& OuterCard = GetTypedOuter<UFaerieCardBase>())
	{
		OuterCard->GetOnCardRefreshed().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UFaerieCardTokenBase::OnCardRefreshed()
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	ItemToken = nullptr;

	if (IsValid(GetTokenClass()))
	{
		if (auto&& OuterCard = GetTypedOuter<UFaerieCardBase>())
		{
			if (auto&& ItemData = OuterCard->GetItemData();
				ItemData.IsValid())
			{
				if (auto&& Object = ItemData->GetItemObject())
				{
					ItemToken = Object->GetToken(GetTokenClass());
				}
			}
		}
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	BP_Refresh();
}

const UFaerieItem* UFaerieCardTokenBase::GetItem() const
{
	if (auto&& Card = GetOwningCard())
	{
		if (auto&& ItemObj = Card->GetStackView().Item;
			ItemObj.IsValid())
		{
			return ItemObj.Get();
		}
	}
	return nullptr;
}

FFaerieItemProxy UFaerieCardTokenBase::GetProxy() const
{
	if (auto&& Card = GetOwningCard())
	{
		return Card->GetItemData();
	}
	return nullptr;
}

const UFaerieItemToken* UFaerieCardTokenBase::GetItemToken() const
{
	if (auto&& ItemObj = GetItem())
	{
		return ItemObj->GetToken(GetTokenClass());
	}
	return nullptr;
}

bool UFaerieCardTokenBase::GetItemTokenChecked(UFaerieItemToken*& Token, TSubclassOf<UFaerieItemToken>) const
{
	// @Note: BP doesn't understand const-ness, but since UFaerieItemToken does not have a BP accessible API that can
	// mutate it, it's perfectly safe.
	Token = const_cast<UFaerieItemToken*>(GetItemToken());
	return IsValid(Token);
}

UFaerieCardBase* UFaerieCardTokenBase::GetOwningCard() const
{
	return GetTypedOuter<UFaerieCardBase>();
}