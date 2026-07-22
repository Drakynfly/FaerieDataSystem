// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/NotNull.h"
#include "UObject/Object.h"

namespace Faerie::Utils
{
	// Explanation: Objects loaded from disk carry certain flags that let them be replicated efficiently.
	// When making a duplicate of that object, those flags would carry over and cause issues with replication.
	// https://forums.unrealengine.com/t/duplicated-uobject-causes-client-to-disconnect/427621

	inline constexpr EObjectFlags LoadFlags = RF_WasLoaded | RF_LoadCompleted;

	// Does this object have a flag that indicates it's an asset loaded from disk?
	UE_REWRITE bool HasLoadFlag(const TNotNull<const UObject*> SourceObject)
	{
		return SourceObject->HasAnyFlags(LoadFlags);
	}

	// Clear load flags from an object
	UE_REWRITE void ClearLoadFlags(const TNotNull<UObject*> SourceObject)
	{
		SourceObject->ClearFlags(LoadFlags);
	}

	template< class T >
	T* DuplicateObjectFromDiskForReplication(T const* SourceObject, UObject* Outer, const FName Name = NAME_None)
	{
		UObject* DuplicatedObject = DuplicateObject_Internal(T::StaticClass(), SourceObject, Outer, Name);
		ClearLoadFlags(DuplicatedObject);
		return static_cast<T*>(DuplicatedObject);
	}

	template <typename T>
	T* DuplicateObjectFromDiskForReplication(const TObjectPtr<const T>& SourceObject, UObject* Outer, const FName Name = NAME_None)
	{
		UObject* DuplicatedObject = DuplicateObject_Internal(T::StaticClass(), SourceObject, Outer, Name);
		ClearLoadFlags(DuplicatedObject);
		return static_cast<T*>(DuplicatedObject);
	}
}
