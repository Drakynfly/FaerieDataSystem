// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

namespace Faerie::Token
{
	enum class EFilterFlags : uint32
	{
		None = 0,

		// This filter is restricted to emitting immutable tokens
		ImmutableOnly = 1 << 0,

		// This filter is restricted to emitting mutable tokens
		MutableOnly = 1 << 1,

		Inverted = 1 << 3
	};
	ENUM_CLASS_FLAGS(EFilterFlags)

	template <EFilterFlags Flags>
	consteval EFilterFlags FlagMutableOnly()
	{
		return (Flags & ~EFilterFlags::ImmutableOnly) | EFilterFlags::MutableOnly;
	}

	template <EFilterFlags Flags>
	consteval EFilterFlags FlagImmutableOnly()
	{
		return (Flags & ~EFilterFlags::MutableOnly) | EFilterFlags::ImmutableOnly;
	}
}
