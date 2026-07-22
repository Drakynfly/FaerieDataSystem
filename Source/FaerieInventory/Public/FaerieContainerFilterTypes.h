// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"
#include "GameplayTagContainer.h"

namespace Faerie::Container
{
	struct FAERIEINVENTORY_API FMatchItemMutable
	{
		bool Exec(TNotNull<const UObject*> WorldContextObj, ItemData::FValidatedDataView View) const;
		bool MutabilityToMatch;
	};

	// Compare the test Item's name against an FText.
	struct FAERIEINVENTORY_API FCompareName
	{
		bool Exec(TNotNull<const UObject*> WorldContextObj, ItemData::FValidatedDataView View) const;
		FText CompareText;
		ETextComparisonLevel::Type ComparisonType;
	};

	// Test for containing a gameplay tag.
	struct FAERIEINVENTORY_API FHasTag
	{
		bool Exec(TNotNull<const UObject*> WorldContextObj, ItemData::FValidatedDataView View) const;
		FGameplayTag Tag;
		bool HasTagExact;
	};

	// Test for containing a gameplay tag.
	struct FAERIEINVENTORY_API FHasAnyTags
	{
		bool Exec(TNotNull<const UObject*> WorldContextObj, ItemData::FValidatedDataView View) const;
		FGameplayTagContainer Tags;
		bool Exact;
	};

	// Test for containing a gameplay tag.
	struct FAERIEINVENTORY_API FHasAllTags
	{
		bool Exec(TNotNull<const UObject*> WorldContextObj, ItemData::FValidatedDataView View) const;
		FGameplayTagContainer Tags;
		bool Exact;
	};

	// Run a callback on the iterator, allowing user code to run arbitrary selection logic.
	struct FAERIEINVENTORY_API FCallbackFilter
	{
		bool Exec(TNotNull<const UObject*> WorldContextObj, ItemData::FValidatedDataView View) const;
		ItemData::FViewPredicate Callback;
	};
}