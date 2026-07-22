// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/EnumClassFlags.h"
#include "FaerieItemDataEnums.generated.h"

// Options to enable Mutability flags on newly instanced items
UENUM(BlueprintType)
enum class EFaerieItemInstancingMutability : uint8
{
	// Determine mutability from the fragments the item is created with.
	Automatic,

	// The item will be mutable even if the fragments don't need it.
	Mutable,

	// The item will be immutable even if the fragments need it. Note that this will interfere with some behavior, so only
	// use if you are sure you want to disable them.
	Immutable,
};

namespace Faerie::ItemData
{
	enum EMassFragmentExportOptions
	{
		None,

		// Exported will only export FFaerieMassFragment derived structs
		OnlyFaerieMassFragments
	};
	ENUM_CLASS_FLAGS(EMassFragmentExportOptions)

	enum class EFormatVersion : int32
	{
		// Uninitialized item. No version yet.
		Empty = -1,

		// Before format versioning was established.
		BeforeCustomVersionWasAdded = 0,

		// Switch from opt-in to making static references the default.
		ReferencedAssetsAreDefault,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
}
