// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "FaerieAssetInfo.h"

#include "FaerieItemSource.generated.h"

enum class EFaerieItemInstancingMutability : uint8;
class UFaerieItem;

USTRUCT()
struct FAERIEITEMDATA_API FFaerieItemInstancingContext
{
	GENERATED_BODY()

public:
	virtual ~FFaerieItemInstancingContext() = default;

	// Flags to mark instances with
	UPROPERTY()
	EFaerieItemInstancingMutability Flags;

	// Children must implement this to allow safe casting.
	virtual const UScriptStruct* GetScriptStruct() const { return FFaerieItemInstancingContext::StaticStruct(); }

	template <typename T>
	const T* Cast() const
	{
		if (GetScriptStruct()->IsChildOf(T::StaticStruct()))
		{
			return static_cast<const T*>(this);
		}
		return nullptr;
	}
};

// This class does not need to be modified.
UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class FAERIEITEMDATA_API UFaerieItemSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface added to classes that can author a faerie item instance
 */
class FAERIEITEMDATA_API IFaerieItemSource
{
	GENERATED_BODY()

public:
	// Can this source create mutable items?
	virtual bool CanBeMutable() const { return false; }

	// Allows sources to give info about generation results
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemSource")
	virtual FFaerieAssetInfo GetSourceInfo() const { return FFaerieAssetInfo(); }

	// Create a item instance from this source.
	// An InstancingContext may be required to provide contextual data from the requester of the item.
	virtual const UFaerieItem* CreateItemInstance(const FFaerieItemInstancingContext* Context) const
		PURE_VIRTUAL(IFaerieItemSource::CreateItemInstance, return nullptr; )
};

/**
 * A wrapper struct that can container a pointer to any object that implements IFaerieItemSource
 * Held reference is soft.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemSourceObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "/Script/FaerieItemData.FaerieItemSource"))
	TSoftObjectPtr<UObject> Object;

	friend bool operator==(const FFaerieItemSourceObject& Lhs, const FFaerieItemSourceObject& Rhs)
	{
		return Lhs.Object == Rhs.Object;
	}

	friend bool operator!=(const FFaerieItemSourceObject& Lhs, const FFaerieItemSourceObject& Rhs)
	{
		return !(Lhs == Rhs);
	}

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieItemSourceObject& Value)
	{
		return GetTypeHash(Value.Object);
	}
};