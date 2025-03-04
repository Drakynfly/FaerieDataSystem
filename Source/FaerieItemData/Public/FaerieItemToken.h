﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "NetSupportedObject.h"
#include "FaerieItemToken.generated.h"

class UFaerieItem;

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBlueprintTokenEdit, UFaerieItemToken*, Token);

/**
 * A fragment of an inventory item, encapsulating a single feature of the item.
 */
UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew, Abstract, CollapseCategories, HideDropdown)
class FAERIEITEMDATA_API UFaerieItemToken : public UNetSupportedObject
{
	GENERATED_BODY()

public:
	// Can the data contained be this token by changed after initialization. This plays a major role on how items are
	// handled. An item with *any* mutable data cannot be stacked.
	virtual bool IsMutable() const;

protected:
	/*
	 * Compare the data of this token to another. Most of the time, there is no need to override this. This function is
	 * used to determine if two items are identical, data-wise, but since only *one* token on an item needs to differ for
	 * the item to be considered distinct, not every token needs to implement this.
	 * Tokens that *should* implement this are ones that are primarily used to identify items, like Name or Info tokens,
	 * or any token that explicitly is used to differentiate items when their primary identifiers match.
	 * Further note that any token that is mutable is automatically dissimilar even if it is data-wise identical, so it
	 * is meaningless to implement this in that case.
	 */
	virtual bool CompareWithImpl(const UFaerieItemToken* Other) const;

	// Are we in an item that is mutable?
	bool IsOuterItemMutable() const;

	void NotifyOuterOfChange();

public:
	UFaerieItem* GetOuterItem() const;

	// Compare the data of this token to another
	bool CompareWith(const UFaerieItemToken* Other) const;

	void EditToken(const TFunctionRef<bool(UFaerieItemToken*)>& EditFunc);

	template <
		typename TFaerieItemToken
		UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
	>
	void EditToken(const TFunctionRef<bool(TFaerieItemToken*)>& EditFunc)
	{
		EditToken(EditFunc);
	}

protected:
	/**		BLUEPRINT API		*/

	/** Get the item this token is part of */
	UFUNCTION(BlueprintCallable, Category = "FaerieItemToken", meta = (DisplayName = "Get Faerie Item"))
	UFaerieItem* BP_GetFaerieItem() const;

	/** Attempt to modify this token. Pass in a predicate that must perform the edit. */
	UFUNCTION(BlueprintCallable, Category = "FaerieItemToken", meta = (DisplayName = "Edit Token"))
	void BP_EditToken(const FBlueprintTokenEdit& Edit);
};