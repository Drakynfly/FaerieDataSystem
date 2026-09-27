// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemEvent.h"

#include "UObject/Interface.h"
#include "FaerieItemDataViewFieldChangeInterface.generated.h"

struct FMassEntityManager;

UINTERFACE()
class UFaerieItemDataViewFieldChangeInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FAERIEITEMDATA_API IFaerieItemDataViewFieldChangeInterface
{
	GENERATED_BODY()

	friend class UFaerieViewModelFieldUpdater;

public:
	virtual FMassEntityHandle GetItemHandle() const
		PURE_VIRTUAL(IFaerieItemDataViewFieldChangeInterface::GetItemHandle, return FMassEntityHandle(); )

protected:
	virtual void OnFieldChange(const FMassEntityManager& EntityManager, const Faerie::ItemData::FFieldChangePayload& Data)
		PURE_VIRTUAL(IFaerieItemDataViewFieldChangeInterface::OnFieldChange, )
};
