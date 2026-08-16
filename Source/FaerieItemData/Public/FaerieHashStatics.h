// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieItemInstance.h"
#include "ValidParameter.h"

struct FMassEntityManager;

namespace Faerie::ItemData
{
	struct FReference;
}

namespace Faerie::Hash
{
	// A function that takes in a UFaerieItem and returns a hash for it.
	using FItemHashFunction = TFunctionRef<uint32(const FMassEntityManager*, TValid<const FFaerieItemInstance&>)>;

	FAERIEITEMDATA_API [[nodiscard]] uint32 Combine(const uint32 A, const uint32 B);

	FAERIEITEMDATA_API [[nodiscard]] FFaerieHash CombineHashes(TArray<uint32>& Hashes);

	// Get the hash of a FProperty's value on a specific object
	FAERIEITEMDATA_API [[nodiscard]] uint32 HashFProperty(TNotNull<const void*> Ptr, const FProperty* Property);

	FAERIEITEMDATA_API [[nodiscard]] uint32 HashStructByProps(TNotNull<const void*> Ptr, TNotNull<const UScriptStruct*> Struct, bool IncludeSuper);
	FAERIEITEMDATA_API [[nodiscard]] uint32 HashObjectByProps(TNotNull<const UObject*> Obj, bool IncludeSuper);

	// Combine the hashes for a set of Items according to a HashFunction
	FAERIEITEMDATA_API [[nodiscard]] FFaerieHash HashItemSet(const FMassEntityManager* EntityManager, const TSet<TValid<const FFaerieItemInstance&>>& Items, const FItemHashFunction& Function);

	// A simple HashFunction that hashes the name of an item by its AssetInfo
	FAERIEITEMDATA_API [[nodiscard]] uint32 HashItemByName(const FMassEntityManager* EntityManager, TValid<const FFaerieItemInstance&> Item);
}