// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieItemDataFwd.h"
#include "GameplayTagContainer.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "StructUtils/InstancedStruct.h"

#include "FaerieItemDataLibrary.generated.h"

struct FFaerieItemInstance;
struct FMassEntityHandle;
class UFaerieItemAsset;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FFaerieItemProxyChangedEvent, const FFaerieItemProxy&, Proxy, FGameplayTag, Tag);

/**
 *
 */
UCLASS()
class UFaerieItemDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static bool IsItemMutable(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static FDateTime GetItemLastModified(const FFaerieItemProxy& Proxy);

	// Get the item instance this asset represents.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static FFaerieUnownedItemStack GetTemplateInstance(const UFaerieItemAsset* Asset);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (AdvancedDisplay = 1))
	static FFaerieUnownedItemStack NewItemInstance(UPARAM(ref) TArray<FInstancedStruct>& Fragments);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static bool HasItemFragment(const FFaerieItemProxy& Proxy, /*TSubScriptStructOf<FFaerieMassFragment>*/ UScriptStruct* FragmentType);

	//UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData")
	//static bool AddFragment(FFaerieItemInstance& Instance, FInstancedStruct Fragment);

	//UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData")
	//static bool RemoveFragment(FFaerieItemInstance& Instance, const UScriptStruct* FragmentType);

	// Gets the fragment of the given type if it exists in the either the asset's defaults or runtime entity.
	// @Todo replace with a version that auto-cast's the output to Fragment type.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemData", meta = (ExpandBoolAsExecs = "ReturnValue"))
	static bool FindFragment(const FFaerieItemInstance& Instance, /*TSubScriptStructOf<FFaerieMassFragment>*/ UScriptStruct* FragmentType, TInstancedStruct<FFaerieMassFragment>& FoundFragment);

	// Gets the fragment of the given type if it exists in the either the asset's defaults or runtime entity.
	// @Todo replace with a version that auto-cast's the output to Fragment type.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemData", meta = (ExpandBoolAsExecs = "ReturnValue", DisplayName = "Find Fragment (Proxy)"))
	static bool FindFragment_Proxy(const FFaerieItemProxy& Proxy, /*TSubScriptStructOf<FFaerieMassFragment>*/ UScriptStruct* FragmentType, TInstancedStruct<FFaerieMassFragment>& FoundFragment);

	// Get the inventory system Unlimited Stack.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData|Macros")
	static int32 UnlimitedStack();

	// Tests if a stack is equal to Unlimited Stack.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData|Macros")
	static bool IsUnlimited(int32 Stack);

	UFUNCTION(BlueprintPure, Category = "Faerie|Hash")
	static int64 BreakFaerieHash(const FFaerieHash Hash) { return Hash.Hash; }

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Copies"))
	static int32 GetViewCopies(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Equal (Faerie Item Proxy)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_ItemProxy(const FFaerieItemProxy& A, const FFaerieItemProxy& B);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Not Equal (Faerie Address)", CompactNodeTitle = "!=", Keywords = "!= not equal compare", ScriptOperator = "!="))
	static bool NotEqual_ItemProxy(const FFaerieItemProxy& A, const FFaerieItemProxy& B);

	// Attempt to cast a weak proxy struct (interface pointer) into a typed proxy object pointer.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData", meta = (DeterminesOutputType = "Class", DynamicOutputParam = "ProxyObject", ExpandBoolAsExecs = "ReturnValue"))
	static bool CastProxy(const FFaerieItemProxy& Proxy, UPARAM(meta = (MustImplement = "/Script/FaerieItemData.FaerieItemDataProxy")) UClass* Class, UObject*& ProxyObject);

	// Make a weak proxy struct from a typed proxy object.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static FFaerieItemProxy ToProxyStruct(const TScriptInterface<IFaerieItemDataProxy>& ProxyObject);

	// Get the Object implementing the IFaerieItemDataProxy interface.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static const UObject* GetProxyObject(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Is Valid (Proxy)"))
	static bool IsValid_ItemProxy(const FFaerieItemProxy& Proxy);

	// Get the number of copies this proxy may access.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Copies"))
	static int32 GetProxyCopies(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData")
	static void BindToItemDataChanged(const FFaerieItemProxy& Proxy, const FFaerieItemProxyChangedEvent& Event);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData")
	static void UnbindAllFromItemDataChanged(const FFaerieItemProxy& Proxy, const UObject* Object);

	/**
	 * Test if an item is mutable
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Is Mutable"))
	static bool ItemIsMutablePredicate(const FFaerieItemProxy& Proxy);

	/**
	 * Test if an item is immutable
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Is Immutable"))
	static bool ItemIsImmutablePredicate(const FFaerieItemProxy& Proxy);

	/**
	 * Compares two items by their name (from info fragment)
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Lexographic Comparison"))
	static bool ItemLexicographicNameComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB);

	/**
	 * Compares two items by their last modified date.
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Last Modified Comparison"))
	static bool ItemDateModifiedComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB);
};