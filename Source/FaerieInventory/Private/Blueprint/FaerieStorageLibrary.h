// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieStorageStructs.h"
#include "FaerieFunctionTemplates.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieStorageLibrary.generated.h"

class UFaerieItem;
class UFaerieItemStackProxy;
class UFaerieItemStorage;

// We need to expose this delegate to the global namespace or UHT will cry.
using FFaerieViewPredicate = UFaerieFunctionTemplates::FFaerieViewPredicate;

/**
 * 
 */
UCLASS()
class UFaerieStorageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "Is Valid (Entry Key)")
    static bool IsValid_EntryKey(const FFaerieEntryKey Key) { return Key.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "Is Valid (Stack Key)")
    static bool IsValid_StackKey(const FFaerieStackKey Key) { return Key.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "To String (Entry Key)")
	static FString ToString_EntryKey(const FFaerieEntryKey Key) { return Key.ToString(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "To String (Stack Key)")
	static FString ToString_StackKey(const FFaerieStackKey Key) { return Key.ToString(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static bool IsValid_Address(const FFaerieAddress Address) { return Address.IsValid(); }

	// @todo not safe to use. Only works for addresses from a UFaerieItemStorage.
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "To String (Faerie Address)")
	static FString ToString_Address(FFaerieAddress Address);

	/* Returns true if inventory keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Equal (Faerie Address)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_Address(const FFaerieAddress A, const FFaerieAddress B) { return A == B; }

	/* Returns true if inventory keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Not Equal (Faerie Address)", CompactNodeTitle = "!=", Keywords = "!= not equal compare", ScriptOperator = "!="))
	static bool NotEqual_Address(const FFaerieAddress A, const FFaerieAddress B) { return A != B; }

	/* Returns true if stack keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Equal (Stack Key)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_StackKey(const FFaerieStackKey A, const FFaerieStackKey B) { return A == B; }

	/* Returns true if stack keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Not Equal (Stack Key)", CompactNodeTitle = "!=", Keywords = "!= not equal compare", ScriptOperator = "!="))
	static bool NotEqual_StackKey(const FFaerieStackKey A, const FFaerieStackKey B) { return A != B; }

	/* Returns true if entry keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Equal (Entry Key)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_EntryKey(const FFaerieEntryKey A, const FFaerieEntryKey B) { return A == B; }

	/* Returns true if entry keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Not Equal (Entry Key)", CompactNodeTitle = "!=", Keywords = "!= not equal compare", ScriptOperator = "!="))
	static bool NotEqual_EntryKey(const FFaerieEntryKey A, const FFaerieEntryKey B) { return A != B; }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static int32 GetItemStackLimit(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Inventory|Utils")
	static bool GetNetworkHandleFromProxy(const FFaerieItemProxy& Proxy, FFaerieItemNetworkHandle& OutHandle);

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static FFaerieItemProxy GetProxyFromNetworkHandle(const FFaerieItemNetworkHandle& Handle);

	// Query function to filter for the first matching entry.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Library")
	static TArray<UFaerieItemStackProxy*> GetAllStackProxies(UFaerieItemStorage* Storage);

	// Query function to filter for the first matching entry.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Library")
	static FFaerieAddress QueryFirst(UFaerieItemStorage* Storage, const UFaerieFunctionTemplates::FFaerieViewPredicate& Filter);

	// Gets the first subobject of the given class
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|Subobject", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "Class", DynamicOutputParam = "FoundContainers", ExpandBoolAsExecs = "ReturnValue"))
	static bool FindSubobject(UObject* WorldContextObject, const FFaerieItemInstance& Instance, TSubclassOf<UFaerieItemContainerBase> Class, UFaerieItemContainerBase*& FoundContainers, const bool Recursive);

	// Gets all subobjects of the given class
	UFUNCTION(BlueprintCallable, Category = "Faerie|SubObjects", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "Class", DynamicOutputParam = "FoundContainers"))
	static void FindSubObjectsByClass(UObject* WorldContextObject, const FFaerieItemInstance& Instance, TSubclassOf<UFaerieItemContainerBase> Class, TArray<UFaerieItemContainerBase*>& FoundContainers, const bool Recursive);

	// Get all container objects.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SubObjects", meta = (WorldContext = "WorldContextObject"))
	static void GetAllContainersInItem(UObject* WorldContextObject, const FFaerieItemInstance& Instance, TArray<UFaerieItemContainerBase*>& FoundContainers, bool Recursive);

	// Get all child items from an item.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SubObjects", meta = (WorldContext = "WorldContextObject"))
	static void GetItemChildren(UObject* WorldContextObject, const FFaerieItemInstance& Instance, TArray<FFaerieItemInstance>& FoundInstances, bool Recursive);
};
