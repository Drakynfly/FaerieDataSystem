// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemTokenFilter.h"
#include "GameplayTagContainer.h"

namespace Faerie::Token
{
	struct FAERIEITEMDATA_API FMutableFilter final : ITokenFilterType
	{
		virtual bool Passes(const UFaerieItemToken* Token) override { return StaticPasses(Token); }
		static bool StaticPasses(const UFaerieItemToken* Token);
	};

	template <>
	struct TFilterTraits<FMutableFilter>
	{
		static constexpr EFilterFlags TypeFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::MutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::ImmutableOnly;
	};

	struct FAERIEITEMDATA_API FImmutableFilter final : ITokenFilterType
	{
		virtual bool Passes(const UFaerieItemToken* Token) override { return StaticPasses(Token); }
		static bool StaticPasses(const UFaerieItemToken* Token);
	};

	template <>
	struct TFilterTraits<FImmutableFilter>
	{
		static constexpr EFilterFlags FilterFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::ImmutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::MutableOnly;
	};

	struct FAERIEITEMDATA_API FTagFilter final : ITokenFilterType
	{
		FTagFilter(const FGameplayTag& Tag, const bool Exact = false)
		  : Tag(Tag), Exact(Exact) {}

		virtual bool Passes(const UFaerieItemToken* Token) override;

	protected:
		const FGameplayTag Tag;
		const bool Exact;
	};

	struct FAERIEITEMDATA_API FTagsFilter final : ITokenFilterType
	{
		FTagsFilter(const FGameplayTagContainer& Tags, const bool All = false, const bool Exact = false)
		  : Tags(Tags), All(All), Exact(Exact) {}

		virtual bool Passes(const UFaerieItemToken* Token) override;

	protected:
		const FGameplayTagContainer Tags;
		const bool All;
		const bool Exact;
	};

	struct FAERIEITEMDATA_API FTagQueryFilter final : ITokenFilterType
	{
		FTagQueryFilter(const FGameplayTagQuery& Query)
		  : Query(Query) {}

		virtual bool Passes(const UFaerieItemToken* Token) override;

	protected:
		const FGameplayTagQuery Query;
	};
}
