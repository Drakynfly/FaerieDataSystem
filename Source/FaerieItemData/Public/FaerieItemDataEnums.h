// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/EnumClassFlags.h"
#include "FaerieItemDataEnums.generated.h"

UENUM(Flags, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFaerieItemMutabilityFlags : uint8
{
	None = 0,

	// 'Instance Mutability' is a required flag for item instances to be changed after construction. Without this flag,
	// items are understood to be forever immutable because it either lives in a package, or other outer that is static
	// and itself cannot be changed, regardless of whether the instance has Token Mutability enabled.
	// Items created at runtime or duplicated from static instances have this flag enabled by default.
	// (See the definitions of CreateInstance / CreateDuplicate)
	InstanceMutability = 1 << 0,

	// 'Token Mutability' says that this item has one or more tokens that request the ability to mutate their internal state.
	// Mutability at runtime is only allowed if InstanceMutability is also enabled.
	TokenMutability = 1 << 1,

	// Enable to make all instances of this item mutable, even if no current Tokens request mutability. This is usually
	// required when making an item template expected to have a mutable token added dynamically at runtime, but
	// doesn't have any mutable tokens added by the editor.
	AlwaysTokenMutable = 1 << 2,

	// Prevent token mutation at runtime, even if TokenMutability is enabled.
	ForbidTokenMutability = 1 << 3,
};
ENUM_CLASS_FLAGS(EFaerieItemMutabilityFlags)

// Options to enable Mutability flags on newly instanced items
UENUM(BlueprintType)
enum class EFaerieItemInstancingMutability : uint8
{
	// Determine mutability from the tokens the item is created with.
	Automatic,

	// The item will be mutable even if the tokens don't need it.
	Mutable,

	// The item will be immutable even if the tokens need it. Note that this will interfere with some behavior, so only
	// use if you are sure you want to disable them.
	Immutable,
};

namespace Faerie
{
	UE_REWRITE EFaerieItemMutabilityFlags ToFlags(const EFaerieItemInstancingMutability Mutability)
	{
		switch (Mutability)
		{
		case EFaerieItemInstancingMutability::Mutable: return EFaerieItemMutabilityFlags::AlwaysTokenMutable;
		case EFaerieItemInstancingMutability::Immutable: return EFaerieItemMutabilityFlags::ForbidTokenMutability;
		case EFaerieItemInstancingMutability::Automatic:
		default: return EFaerieItemMutabilityFlags::None;
		}
	}
}

UENUM(Flags, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFaerieItemComparisonFlags : uint8
{
	None = 0,

	// When enabled, if either item is Mutable, the comparison will return false
	Mutability_TreatAsUnequivocable = 1 << 0,

	// When enabled, skip checking Mutability, only compare tokens
	Mutability_Ignore = 1 << 1,

	// When enabled, items must match mutability and tokens in comparison.
	Mutability_Compare = 1 << 2,

	// When enabled, only compare Primary Identifier tokens via CompareWith override
	Tokens_ComparePrimaryIdentifiers = 1 << 3,

	// When enabled, compare all tokens by their hash
	Tokens_CompareAll = 1 << 4,

	Default = Mutability_TreatAsUnequivocable | Tokens_ComparePrimaryIdentifiers,
	CheckTokensOnly = Mutability_Ignore | Tokens_CompareAll,
};
ENUM_CLASS_FLAGS(EFaerieItemComparisonFlags)