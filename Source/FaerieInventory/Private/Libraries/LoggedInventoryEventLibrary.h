// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LoggedInventoryEventLibrary.generated.h"

struct FEntryKey;
struct FFaerieAddress;
struct FFaerieInventoryTag;
struct FFaerieItemStackView;
struct FLoggedInventoryEvent;

UCLASS()
class ULoggedInventoryEventLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LoggedInventoryEventLibrary", meta = (NativeBreakFunc))
	static void BreakLoggedInventoryEvent(const FLoggedInventoryEvent& LoggedEvent, FFaerieInventoryTag& Type,
										  FDateTime& Timestamp, FEntryKey& EntryTouched,
										  TArray<FFaerieAddress>& AddressesTouched, FFaerieItemStackView& Stack);
};