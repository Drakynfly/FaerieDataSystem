// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataViewWrapper.generated.h"

namespace Faerie::ItemData
{
	class IViewBase;
}

/**
 * A wrapper around an Item Data View pointer for exposing to Blueprint.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemDataViewWrapper
{
	GENERATED_BODY()

	FFaerieItemDataViewWrapper() = default;
	FFaerieItemDataViewWrapper(const TNotNull<const Faerie::ItemData::IViewBase*> Pointer)
	  : ViewPointer(Pointer) {}

	friend bool operator==(const FFaerieItemDataViewWrapper& A, const FFaerieItemDataViewWrapper& B)
	{
		return A.ViewPointer == B.ViewPointer;
	}

	const Faerie::ItemData::IViewBase* ViewPointer;
};

/*
template<>
struct TStructOpsTypeTraits<FFaerieItemDataViewWrapper> : public TStructOpsTypeTraitsBase2<FFaerieItemDataViewWrapper>
{
	enum
	{
		WithCopy = true
	};
};
*/