// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataEnums.h"
#include "FaerieItemInstancingContext.generated.h"

USTRUCT()
struct FAERIEITEMDATA_API FFaerieItemInstancingContext
{
	GENERATED_BODY()

public:
	virtual ~FFaerieItemInstancingContext() = default;

	// Entity manager used to access runtime item fragments.
	FMassEntityManager* EntityManager = nullptr;

	// Number of copies to generate. If unset, will default to 1.
	TOptional<int32> CopiesOverride;

	// Mutability of the instanced item
	EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic;

	// If a caller sets CopiesOverride and expects that a mutable item may be generated, set this to promise that we will duplicate the item.
	bool EmitStackEvenIfMutableBecauseCallerKnowsToDuplicateItem = false;

	// Create an instance that refers to its defaults through a reference fragment rather than an item asset pointer.
	bool CreateReferencingInstance = false;

#if WITH_EDITORONLY_DATA
	// A flag to mark a context as being run by the editor.
	bool RunningInEditor = false;

	// The object that will be used as the outer for new item instances.
	UPROPERTY()
	TObjectPtr<UObject> Editor_ItemInstanceOuter;
#endif

	// Children must implement this to allow safe casting.
	UE_REWRITE virtual const UScriptStruct* GetScriptStruct() const { return FFaerieItemInstancingContext::StaticStruct(); }

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