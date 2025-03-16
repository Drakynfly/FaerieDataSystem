// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataEnums.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "NetSupportedObject.h"
#include "TypeCastingUtils.h"

#include "FaerieItem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFaerieItem, Log, All);

class UFaerieItem;
class UFaerieItemToken;

namespace Faerie
{
	namespace Tags
	{
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenAdd)
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenRemove)
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenGenericPropertyEdit)
	}

	class FTokenFilter : FNoncopyable
	{
		friend class UFaerieItem;

		FTokenFilter(const TArray<TObjectPtr<UFaerieItemToken>>& Tokens)
		  : Tokens(Tokens) {}

	public:
		template <
			typename TFaerieItemToken
			UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
		>
		FTokenFilter& ByClass()
		{
			return ByClass(TFaerieItemToken::StaticClass());
		}

		FAERIEITEMDATA_API FTokenFilter& ByClass(const TSubclassOf<UFaerieItemToken>& Class);

		FAERIEITEMDATA_API FTokenFilter& ByTag(const FGameplayTag& Tag, const bool Exact = false);

		FAERIEITEMDATA_API FTokenFilter& ByTags(const FGameplayTagContainer& Tags, const bool All = false, const bool Exact = false);

		FAERIEITEMDATA_API FTokenFilter& ByTagQuery(const FGameplayTagQuery& Query);

		// Iterates over the filtered tokens. Return true in the delegate to continue iterating.
		FAERIEITEMDATA_API FTokenFilter& ForEach(const TFunctionRef<bool(const TObjectPtr<UFaerieItemToken>&)>& Iter);

		FAERIEITEMDATA_API int32 GetNum() const { return Tokens.Num(); }

		// Resolve into the final token array, but cast into an array of const pointers to prevent mutable pointers leaking out of a const API
		FAERIEITEMDATA_API TArray<const TObjectPtr<UFaerieItemToken>> operator*() const
		{
			return *reinterpret_cast<const TArray<const TObjectPtr<UFaerieItemToken>>*>(&Tokens);
		}

	private:
		TArray<TObjectPtr<UFaerieItemToken>> BlueprintOnlyAccess() { return Tokens;}

		TArray<TObjectPtr<UFaerieItemToken>> Tokens;
	};

	using FNotifyOwnerOfSelfMutation = TDelegate<void(const UFaerieItem*, const class UFaerieItemToken*, FGameplayTag)>;
}

/**
 * A runtime instance of an item.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType)
class FAERIEITEMDATA_API UFaerieItem : public UNetSupportedObject
{
	GENERATED_BODY()

	friend class UFaerieItemToken;
	friend class UFaerieItemAsset;

public:
	//~ Begin UObject interface
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void PostLoad() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetReplicatedCustomConditionState(FCustomPropertyConditionState& OutActiveState) const override;
	//~ Emd UObject interface

	// Iterates over each contained token. Return true in the delegate to continue iterating.
	void ForEachToken(const TFunctionRef<bool(const TObjectPtr<UFaerieItemToken>&)>& Iter) const;

	// Iterates over each contained token. Return true in the delegate to continue iterating.
	void ForEachTokenOfClass(const TFunctionRef<bool(const TObjectPtr<UFaerieItemToken>&)>& Iter, const TSubclassOf<UFaerieItemToken>& Class) const;

	// Iterates over each contained token. Return true in the delegate to continue iterating.
	template <
		typename TFaerieItemToken
		UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
	>
	void ForEachToken(const TFunctionRef<bool(const TObjectPtr<TFaerieItemToken>&)>& Iter) const
	{
		ForEachTokenOfClass(
			*reinterpret_cast<const TFunctionRef<bool(const TObjectPtr<UFaerieItemToken>&)>*>(&Iter)
			, TFaerieItemToken::StaticClass());
	}

	// Creates a new faerie item object with the given tokens. These are instance-mutable by default.
	static UFaerieItem* CreateNewInstance(TConstArrayView<UFaerieItemToken*> Tokens, EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic);

	// Creates a new faerie item object using this instance as a template. Instance-mutable only if required by item or flags.
	UFaerieItem* CreateInstance(EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic) const;

	// Creates a new faerie item object using this instance as a template. Duplicates are instance-mutable by default.
	UFaerieItem* CreateDuplicate(EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic) const;

	TConstArrayView<TObjectPtr<UFaerieItemToken>> GetTokens() const { return Tokens; }

	const UFaerieItemToken* GetToken(const TSubclassOf<UFaerieItemToken>& Class) const;
	TArray<const UFaerieItemToken*> GetTokens(const TSubclassOf<UFaerieItemToken>& Class) const;
	UFaerieItemToken* GetMutableToken(const TSubclassOf<UFaerieItemToken>& Class);
	TArray<UFaerieItemToken*> GetMutableTokens(const TSubclassOf<UFaerieItemToken>& Class);

	template <
		typename TFaerieItemToken
		UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
	>
	const TFaerieItemToken* GetToken() const
	{
		return Cast<TFaerieItemToken>(GetToken(TFaerieItemToken::StaticClass()));
	}

	template <
		typename TFaerieItemToken
		UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
	>
	TArray<const TFaerieItemToken*> GetTokens() const
	{
		return Type::Cast<TArray<const TFaerieItemToken*>>(GetTokens(TFaerieItemToken::StaticClass()));
	}

	template <
		typename TFaerieItemToken
		UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
	>
	TFaerieItemToken* GetEditableToken()
	{
		return Cast<TFaerieItemToken>(GetMutableToken(TFaerieItemToken::StaticClass()));
	}

	template <
		typename TFaerieItemToken
		UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
	>
	TArray<TFaerieItemToken*> GetEditableTokens()
	{
		return Type::Cast<TArray<TFaerieItemToken*>>(GetMutableTokens(TFaerieItemToken::StaticClass()));
	}

	Faerie::FTokenFilter FilterTokens() const;

	/*
	 * Compares two Items to determine if they are "the same". There are some limitations here imposed by the design of
	 * FDS. See EFaerieItemComparisonFlags for descriptions of the Flags.
	 */
	static bool Compare(const UFaerieItem* A, const UFaerieItem* B, const EFaerieItemComparisonFlags Flags);
	bool CompareWith(const UFaerieItem* Other, const EFaerieItemComparisonFlags Flags) const;


	//~		C++ Item Mutation		~//

	// Mutate cast will return a const_cast'd *this* if the item is a runtime mutable instance. This is the proscribed
	// method to gain access to the non-const API of UFaerieItem.
	UFaerieItem* MutateCast() const;
	bool AddToken(UFaerieItemToken* Token);
	bool RemoveToken(UFaerieItemToken* Token);
	int32 RemoveTokensByClass(TSubclassOf<UFaerieItemToken> Class);

protected:
	// While not returning const pointers, these are considered safe, as all mutating functions are locked being FFaerieItemEditHandle

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem")
	TArray<UFaerieItemToken*> GetAllTokens() const;

	// Gets the first token of the given class
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem", meta = (DeterminesOutputType = Class, DynamicOutputParam = FoundToken, ExpandBoolAsExecs = ReturnValue))
	bool FindToken(TSubclassOf<UFaerieItemToken> Class, UFaerieItemToken*& FoundToken) const;

	// Gets all tokens of the given class
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem", meta = (DeterminesOutputType = Class, DynamicOutputParam = FoundTokens))
	void FindTokens(TSubclassOf<UFaerieItemToken> Class, TArray<UFaerieItemToken*>& FoundTokens) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem")
	TArray<UFaerieItemToken*> FindTokensByTag(const FGameplayTag& Tag, const bool Exact = false) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem")
	TArray<UFaerieItemToken*> FindTokensByTags(const FGameplayTagContainer& Tags, const bool All = false, const bool Exact = false) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem")
	TArray<UFaerieItemToken*> FindTokensByTagQuery(const FGameplayTagQuery& Query) const;

	// Deprecated Blueprint functions that have been moved to UFaerieItemDataLibrary.

	UFUNCTION(BlueprintCallable, Category = "FaerieItem", meta = (DeprecatedFunction, DeprecationMessage = "Use UFaerieItemDataLibrary::AddToken"))
	void AddToken(UFaerieItemToken* Token, bool DeprecatedFunctionOverload) { AddToken(Token); }

	UFUNCTION(BlueprintCallable, Category = "FaerieItem", meta = (DeprecatedFunction, DeprecationMessage = "Use UFaerieItemDataLibrary::RemoveToken"))
	bool RemoveToken(UFaerieItemToken* Token, bool DeprecatedFunctionOverload) { return RemoveToken(Token); }

	UFUNCTION(BlueprintCallable, Category = "FaerieItem", meta = (DeprecatedFunction, DeprecationMessage = "Use UFaerieItemDataLibrary::RemoveTokensByClass"))
	int32 RemoveTokensByClass(TSubclassOf<UFaerieItemToken> Class, bool DeprecatedFunctionOverload) { return RemoveTokensByClass(Class); }

public:
	UFUNCTION(BlueprintCallable, Category = "FaerieItem")
	FDateTime GetLastModified() const { return LastModified; }

	/*
	UFUNCTION(BlueprintCallable, Category = "FaerieItem")
	EFaerieItemSourceType GetSourceType() const;
	*/

	// Can this item object be changed whatsoever at runtime? This is not available for asset-referenced or precached items.
	UFUNCTION(BlueprintCallable, Category = "FaerieItem")
	bool IsInstanceMutable() const;

	// Are the tokens in this item capable of being changed at runtime?
	UFUNCTION(BlueprintCallable, Category = "FaerieItem")
	bool IsDataMutable() const;

	// Does this item instance meet all requirements to have its tokens mutated at runtime? See EFaerieItemMutabilityFlags.
	// This is equivalent to IsInstanceMutable && IsDataMutable.
	UFUNCTION(BlueprintCallable, Category = "FaerieItem")
	bool CanMutate() const;

protected:
	// Called by our own tokens when they are edited.
	void OnTokenEdited(const UFaerieItemToken* Token);

	void CacheTokenMutability();

public:
	Faerie::FNotifyOwnerOfSelfMutation::RegistrationType& GetNotifyOwnerOfSelfMutation() { return NotifyOwnerOfSelfMutation; }

protected:
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FaerieItem")
	TArray<TObjectPtr<UFaerieItemToken>> Tokens;

	// Keeps track of the last time this item was modified. Allows, for example, sorting items by recently touched.
	// Only intended to be useful for mutable and dynamically-generated items. Asset-derived instances will be set
	// to the time that they were cooked (at runtime) or last edited (at dev-time).
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FaerieItem")
	FDateTime LastModified = FDateTime();

	// Mutability flags.
	// In order for an item instance to be changed at runtime, it must have no mutually exclusive flags.
	// This is only set once at item creation, and cannot change after.
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FaerieItem")
	EFaerieItemMutabilityFlags MutabilityFlags;

private:
	// Delegate for owners to bind to, for detecting when tokens are mutated outside their knowledge
	Faerie::FNotifyOwnerOfSelfMutation NotifyOwnerOfSelfMutation;

	// Is writing to Tokens locked?
	mutable uint32 WriteLock = 0;
};