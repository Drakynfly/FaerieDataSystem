// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieCardBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardBase)

void UFaerieCardBase::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);
	const ThisClass* This = CastChecked<ThisClass>(InThis);
	TObjectPtr<const UObject> ObjectPtr(ObjectPtrWrap(This->ItemProxy.GetObject()));
	Collector.AddReferencedObject(ObjectPtr);
}

void UFaerieCardBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshOnConstruct)
	{
		Refresh();
	}
}

void UFaerieCardBase::SetItemData(const FFaerieItemProxy InItemProxy, const bool bRefresh)
{
	ItemProxy = InItemProxy;
	if (bRefresh)
	{
		Refresh();
	}
}

FFaerieItemStackView UFaerieCardBase::GetStackView() const
{
	return ItemProxy;
}

void UFaerieCardBase::Refresh()
{
	OnCardRefreshed.Broadcast();
	BP_Refresh();
}
