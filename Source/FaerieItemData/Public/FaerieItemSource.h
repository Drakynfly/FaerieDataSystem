// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "FaerieAssetInfo.h"
#include "FaerieItemDataEnums.h"
#include "FaerieItemStack.h"

#include "FaerieItemSource.generated.h"

enum class EFaerieItemInstancingMutability : uint8;
class UFaerieItem;

USTRUCT()
struct FAERIEITEMDATA_API FFaerieItemInstancingContext
{
	GENERATED_BODY()

public:
	virtual ~FFaerieItemInstancingContext() = default;

	// Mutability of the instanced item
	UPROPERTY()
	EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic;

	// Number of copies to generate. If unset, will default to 1.
	UPROPERTY()
	TOptional<int32> CopiesOverride;

	// Children must implement this to allow safe casting.
	virtual const UScriptStruct* GetScriptStruct() const { return FFaerieItemInstancingContext::StaticStruct(); }

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

	// Create an item stack from this source.
	// An InstancingContext may be required to provide contextual data from the requester of the stack, depending on the implementation.
	virtual TOptional<FFaerieItemStack> CreateItemStack(const FFaerieItemInstancingContext* Context) const
		PURE_VIRTUAL(IFaerieItemSource::CreateItemStack, return NullOpt; )
};

/**
 * A wrapper struct that contains a soft reference to an object implementing IFaerieItemSource
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemSourceObject
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "/Script/FaerieItemData.FaerieItemSource"))
	TSoftObjectPtr<UObject> Object;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemSourceObject& Other) const
	{
		return Object == Other.Object;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemSourceObject& Value)
	{
		return GetTypeHash(Value.Object);
	}
};