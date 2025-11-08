// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "NetSupportedObject.h"
#include "TypeCastingUtils.h"
#include "FaerieItemToken.generated.h"

class UFaerieItem;

/**
 * Per-class data for Faerie Item Tokens
 */
USTRUCT()
struct FFaerieItemTokenSparseClassStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Filtering")
	FGameplayTagContainer ClassTags;
};

namespace Faerie::Tags
{
	// Tag assigned to "Primary" identifiers. Primary tokens are used to distinguish items without doing full comparisons.
	// See UFaerieItem::CompareWith
	FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(PrimaryIdentifierToken);
}

/**
 * A fragment of an inventory item, encapsulating a single feature of the item.
 */
UCLASS(BlueprintType, Blueprintable, DefaultToInstanced, EditInlineNew, Abstract,
	SparseClassDataTypes = "FaerieItemTokenSparseClassStruct", CollapseCategories, HideDropdown, meta = (DontUseGenericSpawnObject = "True"))
class FAERIEITEMDATA_API UFaerieItemToken : public UNetSupportedObject
{
	GENERATED_BODY()

public:
	//~ Begin UObject interface
#if WITH_EDITOR
	virtual void PostCDOCompiled(const FPostCDOCompiledContext& Context) override;
#endif
	//~ End UObject interface

	// Can the data contained be this token by changed after initialization. This plays a major role in how items are
	// handled. An item with *any* mutable data cannot be stacked.
	virtual bool IsMutable() const;

protected:
	/*
	 * Compare the data of this token to another. Most of the time, there is no need to override this. This function is
	 * used to determine if two items are identical, data-wise, but since only *one* token on an item needs to differ for
	 * the item to be considered distinct, not every token needs to implement this.
	 * Tokens that *should* implement this are ones that are primarily used to identify items, like Name or Info tokens,
	 * or any token that explicitly is used to differentiate items when their primary identifiers match.
	 * Further note that any token that is mutable is automatically dissimilar even if it is data-wise identical, so it
	 * is meaningless to implement this in that case.
	 */
	virtual bool CompareWithImpl(const UFaerieItemToken* Other) const;

	/*
	 * Combine the data of this token into a hash. Most of the time, there is no need to override this. This function is
	 * used to create hashes from one or more tokens for determinism or checksum tests.
	 * Usually, only tokens that are a primary identifier need to implement this. Unlike CompareWithImpl, this function
	 * is still necessary for mutable tokens.
	 */
	virtual uint32 GetTokenHashImpl() const { return 0; }

	// Are we in an item that is mutable?
	bool IsOuterItemMutable() const;

	// This is a utility provided to replicate all the properties of a token class using COND_InitialOnly. This is
	// typically all that an immutable token needs to call in GetLifetimeReplicatedProps.
	void ReplicateAllPropertiesInitialOnly(TArray<class FLifetimeProperty>& OutLifetimeProps) const;

	void NotifyOuterOfChange();

public:
	/**
	 * Gets the item this token is owned by.
	 * WARNING: For immutable tokens, this will often return the prototype item, which is *not* mutable. This function
	 * is only guaranteed to return an instanced (and mutable) item pointer if called on a mutable token.
	 */
	const UFaerieItem* GetOuterItem() const;

	// Compare the data of this token to another.
	static bool Compare(const UFaerieItemToken* A, const UFaerieItemToken* B);
	bool CompareWith(const UFaerieItemToken* Other) const;

	// Get the data of this token as a hash.
	uint32 GetTokenHash() const;


	//~		C++ Token Mutation		~//

	// Mutate cast will return a const_cast'd *this* if the item is a runtime mutable instance. This is the proscribed
	// method to gain access to the non-const API of UFaerieItemToken.
	[[nodiscard]] UFaerieItemToken* MutateCast() const;

	template <Faerie::CItemToken T>
	T* MutateCast() const
	{
		return Cast<T>(MutateCast());
	}

	void EditToken(const TFunctionRef<bool(UFaerieItemToken*)>& EditFunc);

	template <Faerie::CItemToken T>
	void EditToken(const TFunctionRef<bool(T*)>& EditFunc)
	{
		EditToken(Type::Cast<TFunctionRef<bool(UFaerieItemToken*)>>(EditFunc));
	}

protected:
	/**		BLUEPRINT API		*/

	/**
	 * Gets the item this token is owned by.
	 * WARNING: For immutable tokens, this will often return the prototype item, which is *not* mutable. This function
	 * is only guaranteed to return an instanced (and mutable) item pointer if called on a mutable token.
	 */
	UFUNCTION(BlueprintCallable, Category = "FaerieItemToken", meta = (DisplayName = "Get Faerie Item"))
	const UFaerieItem* BP_GetFaerieItem() const;

	friend class UK2Node_CreateFaerieItemToken;
	UFUNCTION(BlueprintCallable, Category = "InternalUseOnly", meta = (BlueprintInternalUseOnly = "true"))
	static UFaerieItemToken* CreateFaerieItemToken(TSubclassOf<UFaerieItemToken> TokenClass);
};