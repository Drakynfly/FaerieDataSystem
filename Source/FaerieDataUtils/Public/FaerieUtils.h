// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

namespace Faerie
{
	// Explanation: Objects loaded from disk carry certain flags that let them be replicated efficiently.
	// When making a duplicate of that object, those flags would carry over and cause issues with replication.
	// https://forums.unrealengine.com/t/duplicated-uobject-causes-client-to-disconnect/427621

	template< class T >
	T* DuplicateObjectFromDiskForReplication(T const* SourceObject, UObject* Outer, const FName Name = NAME_None)
	{
		constexpr EObjectFlags FlagsToClear = RF_WasLoaded | RF_LoadCompleted;
		UObject* DuplicatedObject = DuplicateObject_Internal(T::StaticClass(), SourceObject, Outer, Name);
		DuplicatedObject->ClearFlags(FlagsToClear);
		return static_cast<T*>(DuplicatedObject);
	}

	template <typename T>
	T* DuplicateObjectFromDiskForReplication(const TObjectPtr<const T>& SourceObject, UObject* Outer, const FName Name = NAME_None)
	{
		constexpr EObjectFlags FlagsToClear = RF_WasLoaded | RF_LoadCompleted;
		UObject* DuplicatedObject = DuplicateObject_Internal(T::StaticClass(), SourceObject, Outer, Name);
		DuplicatedObject->ClearFlags(FlagsToClear);
		return static_cast<T*>(DuplicatedObject);
	}
}
