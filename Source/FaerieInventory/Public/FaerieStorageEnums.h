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
enum class EFaerieContainerAddStackCheckType : uint8
{
	// State and Config check.
	// Tests if the stacks can be added in the current state.
	CanAddNow,

	// Config-only check.
	// Only tests if the stacks *could* ever pass the configuration requirement, but not current state.
	CouldEverAdd
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