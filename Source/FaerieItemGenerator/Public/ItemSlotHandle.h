// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/NameTypes.h"
#include "ItemSlotHandle.generated.h"

USTRUCT(BlueprintType, meta=(DisableSplitPin))
struct FAERIEITEMGENERATOR_API FFaerieItemSlotHandle
{
	GENERATED_BODY()

	FFaerieItemSlotHandle() = default;

	FFaerieItemSlotHandle(const FName Name)
	  : InternalHandle(Name) {}

private:
	UPROPERTY(EditAnywhere)
	FName InternalHandle;

public:
	bool IsValid() const
	{
		return !InternalHandle.IsNone();
	}

	FString ToString() const
	{
		return InternalHandle.ToString();
	}

	friend bool operator==(const FFaerieItemSlotHandle& Lhs, const FFaerieItemSlotHandle& Rhs)
	{
		return Lhs.InternalHandle == Rhs.InternalHandle;
	}

	friend bool operator!=(const FFaerieItemSlotHandle& Lhs, const FFaerieItemSlotHandle& Rhs)
	{
		return !(Lhs == Rhs);
	}

	friend uint32 GetTypeHash(const FFaerieItemSlotHandle& Key)
	{
		return GetTypeHash(Key.InternalHandle);
	}
};