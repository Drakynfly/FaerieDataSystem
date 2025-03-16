// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemEditHandle.generated.h"

struct FFaerieItemEditHandle;
class IFaerieItemDataProxy;
class UFaerieItem;

/**
 * This is a wrapper around a UFaerieItem pointer, to provide access to editing a mutable item.
 * This is to provide safety around the editing API, that should not be callable directly on UFaerieItem pointers in BP,
 * due to a lack of const safety in Blueprint graphs.
 *
 * This struct is creatable via UFaerieItemDataLibrary::TryGetEditHandle
 * Do not serialize this struct, it is only designed for runtime use.
 * The pointer is held as weak, emphasizing that this is only a handle for the Blueprint API, not valid storage for an item.
 */
USTRUCT(BlueprintType, meta = (DisableSplitPin,
	HasNativeMake = "/Script/FaerieItemData.FaerieItemDataLibrary.TryGetEditHandle",
	HasNativeBreak = "/Script/FaerieItemData.FaerieItemDataLibrary.GetItem"))
struct FAERIEITEMDATA_API FFaerieItemEditHandle
{
	GENERATED_BODY()

	FFaerieItemEditHandle() = default;

	// Construct from a const pointer. If possible to mutate, this will be const_cast into a mutable pointer.
	FFaerieItemEditHandle(const UFaerieItem* InItem);
	FFaerieItemEditHandle(const IFaerieItemDataProxy* InProxy);

protected:
	UPROPERTY()
	TWeakObjectPtr<UFaerieItem> Item = nullptr;

public:
	UFaerieItem* GetItem() const { return Item.Get(); }

	UFaerieItem* operator->() const { return GetItem(); }

	bool IsValid() const;
};
