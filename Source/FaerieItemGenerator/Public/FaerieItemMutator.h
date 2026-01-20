// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStack.h"
#include "UObject/Object.h"
#include "FaerieItemMutator.generated.h"

class USquirrel;

USTRUCT()
struct FFaerieItemMutatorContext
{
	GENERATED_BODY()

	virtual ~FFaerieItemMutatorContext() = default;

	UPROPERTY()
	TObjectPtr<USquirrel> Squirrel;

	// Children must implement this to allow safe casting.
	virtual const UScriptStruct* GetScriptStruct() const { return FFaerieItemMutatorContext::StaticStruct(); }

	template <typename T>
	const T* Cast() const
	{
		if (GetScriptStruct()->IsChildOf<T>())
		{
			return static_cast<const T*>(this);
		}
		return nullptr;
	}

	template <typename T>
	T* Cast()
	{
		if (GetScriptStruct()->IsChildOf<T>())
		{
			return static_cast<T*>(this);
		}
		return nullptr;
	}
};

/**
 * Base struct for mutation behavior. This functions like a 'command' class, but is implemented as a struct, so that
 * clients can create and RPC these to the server.
 * GetRequiredAssets() is optional to implement.
 * Apply() must be implemented.
 */
USTRUCT()
struct FFaerieItemMutator
{
	GENERATED_BODY()

	virtual ~FFaerieItemMutator() = default;

	// Any soft assets required to be loaded when Apply is called should be registered here.
	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const {}

	//
	virtual bool Apply(FFaerieItemStack& Stack, FFaerieItemMutatorContext* Context) const PURE_VIRTUAL(FFaerieItemMutator::Apply, return false; )
};