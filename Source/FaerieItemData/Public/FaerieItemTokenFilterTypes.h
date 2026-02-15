// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

class UFaerieItem;
class UFaerieItemToken;

namespace Faerie::Token
{
	struct FAERIEITEMDATA_API FMatchMutability
	{
		bool Exec(const TNotNull<const UFaerieItemToken*> Token) const;
		bool MutabilityToMatch;
	};

	struct FAERIEITEMDATA_API FIsOwned
	{
		FIsOwned(const TNotNull<UFaerieItem*> Item)
		  : Item(Item) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		TNotNull<UFaerieItem*> Item;
	};

	struct FAERIEITEMDATA_API FIsClass
	{
		FIsClass(const TSubclassOf<UFaerieItemToken>& Class)
		  : Class(Class) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		TSubclassOf<UFaerieItemToken> Class;
	};

	struct FAERIEITEMDATA_API FHasInterface
	{
		FHasInterface(const TSubclassOf<UInterface>& Class)
		  : Class(Class) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		TSubclassOf<UInterface> Class;
	};

	struct FAERIEITEMDATA_API FIsClassExact
	{
		FIsClassExact(const TSubclassOf<UFaerieItemToken>& Class)
		  : Class(Class) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		TSubclassOf<UFaerieItemToken> Class;
	};

	struct FAERIEITEMDATA_API FIsAnyClass
	{
		FIsAnyClass(const TConstArrayView<TSubclassOf<UFaerieItemToken>> Classes)
		  : Classes(Classes) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		TArray<TSubclassOf<UFaerieItemToken>> Classes;
	};

	struct FAERIEITEMDATA_API FIsAnyClassExact
	{
		FIsAnyClassExact(const TConstArrayView<TSubclassOf<UFaerieItemToken>> Classes)
		  : Classes(Classes) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		TArray<TSubclassOf<UFaerieItemToken>> Classes;
	};

	struct FAERIEITEMDATA_API FTagFilter
	{
		FTagFilter(const FGameplayTag& Tag, const bool Exact = false)
		  : Tag(Tag), Exact(Exact) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		FGameplayTag Tag;
		bool Exact;
	};

	struct FAERIEITEMDATA_API FTagsFilter
	{
		FTagsFilter(const FGameplayTagContainer& Tags, const bool All = false, const bool Exact = false)
		  : Tags(Tags), All(All), Exact(Exact) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		FGameplayTagContainer Tags;
		bool All;
		bool Exact;
	};

	struct FAERIEITEMDATA_API FTagQueryFilter
	{
		FTagQueryFilter(const FGameplayTagQuery& Query)
		  : Query(Query) {}

		bool Exec(TNotNull<const UFaerieItemToken*> Token) const;

	protected:
		FGameplayTagQuery Query;
	};
}
