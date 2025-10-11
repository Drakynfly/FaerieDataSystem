// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"

namespace Faerie::Token
{
	class IFilter;
}

class UFaerieItem;

namespace Faerie::Hash
{
	// A function that takes in a UFaerieItem and returns a hash for it.
	using FItemHashFunction = TFunctionRef<uint32(const UFaerieItem*)>;

	FAERIEITEMDATA_API [[nodiscard]] uint32 Combine(const uint32 A, const uint32 B);

	FAERIEITEMDATA_API [[nodiscard]] FFaerieHash CombineHashes(TArray<uint32>& Hashes);

	// Get the hash of a FProperty's value on a specific object
	FAERIEITEMDATA_API [[nodiscard]] uint32 HashFProperty(const void* Ptr, const FProperty* Property);

	FAERIEITEMDATA_API [[nodiscard]] uint32 HashStructByProps(const void* Ptr, const UScriptStruct* Struct, bool IncludeSuper);
	FAERIEITEMDATA_API [[nodiscard]] uint32 HashObjectByProps(const UObject* Obj, bool IncludeSuper);

	// Combine the hashes for a set of Items according to a HashFunction
	FAERIEITEMDATA_API [[nodiscard]] FFaerieHash HashItemSet(const TSet<const UFaerieItem*>& Items, const FItemHashFunction& Function);

	// A simple HashFunction that hashes the name of an item by its InfoToken
	FAERIEITEMDATA_API [[nodiscard]] uint32 HashItemByName(const UFaerieItem* Item);

	// A HashFunction that hashes the name of an item a set of tokens
	FAERIEITEMDATA_API [[nodiscard]] uint32 HashItemByTokens(const Token::IFilter& Filter);
}