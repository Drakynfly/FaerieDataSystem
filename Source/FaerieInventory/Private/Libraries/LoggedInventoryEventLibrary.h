// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LoggedInventoryEventLibrary.generated.h"

struct FEntryKey;
struct FFaerieInventoryTag;
struct FFaerieItemStackView;
struct FLoggedInventoryEvent;
struct FStackKey;

UCLASS()
class ULoggedInventoryEventLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LoggedInventoryEventLibrary", meta = (NativeBreakFunc))
	static void BreakLoggedInventoryEvent(const FLoggedInventoryEvent& LoggedEvent, FFaerieInventoryTag& Type, bool& Success,
										  FDateTime& Timestamp, FEntryKey& EntryTouched, TArray<FStackKey>& StackKeys,
										  FFaerieItemStackView& Stack, FString& ErrorMessage);
};