// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "InventoryDataEnums.generated.h"

/**
 * This enum holds the flags to bitwise equivalate inventory entries.
 */
UENUM(BlueprintType, Flags, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EEntryEquivalencyFlags : uint8
{
	None = 0 UMETA(Hidden),

	// When set, all generated data will be checked.
	Test_ItemData = 1 << 0 UMETA(DisplayName = "Item Data"),

	// When set, stack sums will be checked.
	Test_StackSum = 1 << 1 UMETA(DisplayName = "Stack Sum"),

	// When set, stack limit will be checked.
	Test_Limit = 1 << 2 UMETA(DisplayName = "Limit"),

	// When set, all other flags are considered set.
	All = Test_ItemData | Test_StackSum | Test_Limit UMETA(Hidden),
};
ENUM_CLASS_FLAGS(EEntryEquivalencyFlags)

UENUM(BlueprintType)
enum class EFaerieStorageAddStackBehavior : uint8
{
	// Add to existing stacks, if possible, and overflow to new stacks
	AddToAnyStack,

	// Don't add to existing stacks, only make new stacks
	OnlyNewStacks
};

UENUM(BlueprintType)
enum class EFaerieStorageAddStackTestMultiType : uint8
{
	// Test if each stack can be added individually.
	IndividualTests,

	// Test if all stacks can be added at once.
	GroupTest
};

UENUM(BlueprintType)
enum class EFaerieItemEqualsCheck : uint8
{
	// Compare only the UFaerieItem pointer.
	ComparePointers,

	// Use UFaerieItem::CompareWith to determine equivalency
	UseCompareWith
};

UENUM(BlueprintType)
enum class EFaerieAddressEventType : uint8
{
	// Broadcast whenever an address is added, or a stack amount is increased.
	PostAdd,

	// Broadcast whenever an address is removed entirely, or a stack amount is decreased.
	PreRemove,

	// Broadcast whenever data for an address is changed.
	Edit
};