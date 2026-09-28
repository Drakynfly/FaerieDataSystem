// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieItemProxy.h"
#include "ValidParameter.h"

#define FAE_API FAERIEITEMDATA_API

struct FMassEntityManager;

namespace Faerie::ItemData
{
	struct FReference;
}

namespace Faerie::Hash
{
	// A function that takes in a UFaerieItem and returns a hash for it.
	using FItemHashFunction = TFunctionRef<uint32(const FMassEntityManager*, TValid<const FFaerieItemProxy&>)>;

	[[nodiscard]] FAE_API uint32 Combine(const uint32 A, const uint32 B);

	[[nodiscard]] FAE_API FFaerieHash CombineHashes(TArrayView<uint32> Hashes);

	// Get the hash of a FProperty's value on a specific object
	[[nodiscard]] FAE_API uint32 HashFProperty(TNotNull<const void*> Ptr, const FProperty* Property);

	[[nodiscard]] FAE_API uint32 HashStructByProps(TNotNull<const void*> Ptr, TNotNull<const UScriptStruct*> Struct, bool IncludeSuper);
	[[nodiscard]] FAE_API uint32 HashObjectByProps(TNotNull<const UObject*> Obj, bool IncludeSuper);

	// A simple HashFunction that hashes the name of an item by its AssetInfo
	[[nodiscard]] FAE_API uint32 HashItemByName(const FMassEntityManager* EntityManager, TValid<const FFaerieItemProxy&> Item);
}

#undef FAE_API