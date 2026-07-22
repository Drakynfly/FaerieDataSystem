// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieStorageEnums.generated.h"

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
enum class EFaerieAddressEventType : uint8
{
	// Broadcast whenever an address is added, or a stack amount is increased.
	PostAdd,

	// Broadcast whenever an address is removed entirely, or a stack amount is decreased.
	PreRemove,

	// Broadcast whenever data for an address is changed.
	Edit
};