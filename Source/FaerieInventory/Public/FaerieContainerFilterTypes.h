// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerIterator.h"
#include "GameplayTagContainer.h"

namespace Faerie::Container
{
	struct FAERIEINVENTORY_API FMatchItemMutable
	{
		bool Exec(FIteratorPtr Iterator) const;
		bool MutabilityToMatch;
	};

	// Compare the test Item's name against an FText.
	struct FAERIEINVENTORY_API FCompareName
	{
		bool Exec(FIteratorPtr Iterator) const;
		FText CompareText;
		ETextComparisonLevel::Type ComparisonType;
	};

	// Test for containing a gameplay tag.
	struct FAERIEINVENTORY_API FHasTag
	{
		bool Exec(FIteratorPtr Iterator) const;
		FGameplayTag Tag;
		bool HasTagExact;
	};

	// Run a callback on the iterator, allowing user code to run arbitrary selection logic.
	struct FAERIEINVENTORY_API FCallbackFilter
	{
		bool Exec(FIteratorPtr Iterator) const;
		FIteratorPredicate Callback;
	};
}