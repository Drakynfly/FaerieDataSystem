// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilter.h"

class UFaerieItemDataFilter;

namespace Faerie
{
	struct FAERIEINVENTORY_API FSingleKey final : IEntryKeyFilter
	{
		FSingleKey() = default;
		explicit FSingleKey(const FEntryKey EntryKey) : TestKey(EntryKey) {}
		virtual bool Passes(FEntryKey Key) override;
		FEntryKey TestKey;
	};


	struct FAERIEINVENTORY_API FKeySet final : IEntryKeyFilter
	{
		virtual bool Passes(FEntryKey Key) override;
		TSet<FEntryKey> TestKeys;
	};


	struct FAERIEINVENTORY_API FMutableFilter final : IItemDataFilter
	{
		virtual bool Passes(const UFaerieItem* Item) override { return StaticPasses(Item); }
		static bool StaticPasses(const UFaerieItem* Item);
	};

	template <>
	struct TFilterProperties<FMutableFilter>
	{
		static constexpr EFilterFlags TypeFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::MutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::ImmutableOnly;
	};


	struct FAERIEINVENTORY_API FImmutableFilter final : IItemDataFilter
	{
		virtual bool Passes(const UFaerieItem* Item) override { return StaticPasses(Item); }
		static bool StaticPasses(const UFaerieItem* Item);
	};

	template <>
	struct TFilterProperties<FImmutableFilter>
	{
		static constexpr EFilterFlags TypeFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::ImmutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::MutableOnly;
	};


	struct FAERIEINVENTORY_API FSnapshotFilterObj : ISnapshotFilter
	{
		virtual bool Passes(const FFaerieItemSnapshot& Snapshot) override;
		UFaerieItemDataFilter* FilterObj;
	};

	struct FAERIEINVENTORY_API FSnapshotFilterCallback : ISnapshotFilter
	{
		virtual bool Passes(const FFaerieItemSnapshot& Snapshot) override;
		FSnapshotFilter Callback;
	};
}