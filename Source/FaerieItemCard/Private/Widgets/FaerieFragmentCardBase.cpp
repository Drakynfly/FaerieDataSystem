// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieFragmentCardBase.h"
#include "Widgets/FaerieCardBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieFragmentCardBase)

void UFaerieFragmentCardBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (auto&& OuterCard = GetTypedOuter<UFaerieCardBase>())
	{
		OuterCard->GetOnCardRefreshed().AddUObject(this, &ThisClass::OnCardRefreshed);
	}
}

void UFaerieFragmentCardBase::NativeDestruct()
{
	if (auto&& OuterCard = GetTypedOuter<UFaerieCardBase>())
	{
		OuterCard->GetOnCardRefreshed().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UFaerieFragmentCardBase::OnCardRefreshed()
{
	BP_Refresh();
}

UFaerieCardBase* UFaerieFragmentCardBase::GetOwningCard() const
{
	return GetTypedOuter<UFaerieCardBase>();
}

FFaerieItemProxy UFaerieFragmentCardBase::GetProxy() const
{
	if (auto&& Card = GetOwningCard())
	{
		return Card->GetItemData();
	}
	return FFaerieItemProxy();
}