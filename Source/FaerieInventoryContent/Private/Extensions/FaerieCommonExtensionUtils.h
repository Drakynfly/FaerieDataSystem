// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GridLayout/FaerieGridStructs.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieCommonExtensionUtils.generated.h"

struct FFaerieAddress;
struct FFaerieContainerGridReadContext;
struct FFaerieEntryKey;
struct FFaerieItemProxy;
class UFaerieItemContainerCapacityView;
class UFaerieContainerContentHashView;
class UFaerieContainerEventLogView;
class UFaerieItemContainerBase;
class UFaerieItemStorage;

/**
 * 
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UFaerieCommonExtensionUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Gets the number of items that be held in a container. Returns -1 if there is no limit.
	UFUNCTION(BlueprintPure, Category = "Faerie|CommonExtensionUtils")
	static int32 GetContainerStackLimit(UFaerieItemContainerBase* Container);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static UFaerieItemContainerCapacityView* GetOrCreateCapacityView(UFaerieItemContainerBase* Container);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static UFaerieContainerEventLogView* GetOrCreateContainerEventLogView(UFaerieItemContainerBase* Container);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static UFaerieContainerContentHashView* GetOrCreateContainerHashView(UFaerieItemContainerBase* Container);

	////////////////////////////////////////

	// View the stack on a specified position on the grid.
	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static FFaerieItemProxy ViewAt(const FFaerieContainerGridReadContext& GridContext, const FIntPoint& Position);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static bool IsCellOccupied(const FFaerieContainerGridReadContext& GridContext, const FIntPoint& Point);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static FFaerieGridPlacement GetStackPlacementData(const FFaerieContainerGridReadContext& GridContext, FFaerieAddress Address);
};
