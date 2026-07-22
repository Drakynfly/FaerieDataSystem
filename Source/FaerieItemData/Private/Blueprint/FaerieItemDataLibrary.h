// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieItem.h"
#include "FaerieItemDataFwd.h"
#include "GameplayTagContainer.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "StructUtils/InstancedStruct.h"

#include "FaerieItemDataLibrary.generated.h"

struct FFaerieItemDataView;
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
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", DisplayName = "Is Valid (Instance)")
	static bool IsValid_ItemInstance(const FFaerieItemInstance& Instance);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static bool EqualEqual_ItemInstance(const FFaerieItemInstance& A, const FFaerieItemInstance& B);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static bool IsItemMutable(const FFaerieItemInstance& Item);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static FDateTime GetItemLastModified(const FFaerieItemInstance& Item);

	// Get the item instance this asset represents.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static FFaerieItemInstance GetItemInstance(const UFaerieItemAsset* Asset);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (AdvancedDisplay = 1, WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static FFaerieItemInstance NewItemInstance(UObject* WorldContextObject, UPARAM(ref) TArray<FInstancedStruct>& Fragments);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static bool HasItemFragment(UObject* WorldContextObject, const FFaerieItemInstance& Instance, /*TSubScriptStructOf<FFaerieMassFragment>*/ UScriptStruct* FragmentType);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static bool AddFragment(UObject* WorldContextObject, FFaerieItemInstance& Instance, FInstancedStruct Fragment);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static bool RemoveFragment(UObject* WorldContextObject, FFaerieItemInstance& Instance, const UScriptStruct* FragmentType);

	// Gets the fragment of the given type if it exists in the either the asset's defaults or runtime entity.
	// @Todo replace with a version that auto-cast's the output to Fragment type.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemData", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", ExpandBoolAsExecs = "ReturnValue"))
	static bool FindFragment(UObject* WorldContextObject, const FFaerieItemInstance& Instance, /*TSubScriptStructOf<FFaerieMassFragment>*/ UScriptStruct* Struct, TInstancedStruct<FFaerieMassFragment>& FoundFragment);

	// Get the inventory system Unlimited Stack.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData|Macros")
	static int32 UnlimitedStack();

	// Tests if a stack is equal to Unlimited Stack.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData|Macros")
	static bool IsUnlimited(int32 Stack);

	UFUNCTION(BlueprintPure, Category = "Faerie|Hash")
	static int64 BreakFaerieHash(const FFaerieHash Hash) { return Hash.Hash; }

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Object"))
	static FFaerieItemInstance GetViewItem(const FFaerieItemDataView& View);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Copies"))
	static int32 GetViewCopies(const FFaerieItemDataView& View);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Owner"))
	static TScriptInterface<IFaerieItemOwnerInterface> GetViewOwner(const FFaerieItemDataView& View);

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

	// Get the Item Instance UObject that this proxy represents. Failable, as not all instances have an Item.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Instance"))
	static FFaerieItemInstance GetProxyItemInstance(const FFaerieItemProxy& Proxy);

	// Get the Proxy Interface Object that points to the item this proxy represents.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Owner"))
	static TScriptInterface<IFaerieItemOwnerInterface> GetProxyItemOwner(const FFaerieItemProxy& Proxy);

	// Get the number of copies this proxy may access.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData", meta = (DisplayName = "Get Item Copies"))
	static int32 GetProxyCopies(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData")
	static void BindToItemDataChanged(const FFaerieItemProxy& Proxy, const FFaerieItemProxyChangedEvent& Event);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemData")
	static void UnbindAllFromItemDataChanged(const FFaerieItemProxy& Proxy, const UObject* Object);

	// Convert an Item Proxy into a Stack View.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemData")
	static FFaerieItemDataView ProxyToView(const FFaerieItemProxy& Proxy);

	/**
	 * Test if an item is mutable
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Is Mutable", WorldContext = "WorldContextObj"))
	static bool ItemIsMutablePredicate(UObject* WorldContextObj, const FFaerieItemDataView& View);

	/**
	 * Test if an item is immutable
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Is Immutable", WorldContext = "WorldContextObj"))
	static bool ItemIsImmutablePredicate(UObject* WorldContextObj, const FFaerieItemDataView& View);

	/**
	 * Compares two items by their name (from info fragment)
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Lexographic Comparison", WorldContext = "WorldContextObj"))
	static bool ItemLexicographicNameComparator(UObject* WorldContextObj, const FFaerieItemDataView& ViewA, const FFaerieItemDataView& ViewB);

	/**
	 * Compares two items by their last modified date.
	 */
	UFUNCTION(/* Item Data Predicate */ meta = (DisplayName = "Last Modified Comparison", WorldContext = "WorldContextObj"))
	static bool ItemDateModifiedComparator(UObject* WorldContextObj, const FFaerieItemDataView& ViewA, const FFaerieItemDataView& ViewB);
};