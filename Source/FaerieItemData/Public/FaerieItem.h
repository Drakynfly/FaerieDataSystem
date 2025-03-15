// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataEnums.h"
#include "FaerieItemToken.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "TypeCastingUtils.h"

#include "FaerieItem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFaerieItem, Log, All);

using FNotifyOwnerOfSelfMutation = TDelegate<void(const class UFaerieItem*, const class UFaerieItemToken*, FGameplayTag)>;

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
	public:
		FTokenFilter(const TArray<TObjectPtr<UFaerieItemToken>>& Tokens)
		  : Tokens(Tokens) {}

		template <
			typename TFaerieItemToken
			UE_REQUIRES(TIsDerivedFrom<TFaerieItemToken, UFaerieItemToken>::Value)
		>
		FTokenFilter& ByClass()
		{
			return ByClass(TFaerieItemToken::StaticClass());
		}

		FTokenFilter& ByClass(const TSubclassOf<UFaerieItemToken>& Class);

		FTokenFilter& ByTag(const FGameplayTag& Tag, const bool Exact = false);

		FTokenFilter& ByTags(const FGameplayTagContainer& Tags, const bool All = false, const bool Exact = false);

		FTokenFilter& ByTagQuery(const FGameplayTagQuery& Query);

		// Iterates over the filtered tokens. Return true in the delegate to continue iterating.
		FTokenFilter& ForEach(const TFunctionRef<bool(const TObjectPtr<UFaerieItemToken>&)>& Iter);

		int32 GetNum() const { return Tokens.Num(); }

		// Resolve into the final token array, but cast into an array of const pointers to prevent mutable pointers leaking out of a const API
		TArray<const TObjectPtr<UFaerieItemToken>> operator*() const
		{
			return *static_cast<const TArray<const TObjectPtr<UFaerieItemToken>>*>(reinterpret_cast<const void*>(&Tokens));
		}

	private:
		TArray<TObjectPtr<UFaerieItemToken>> Tokens;
	};
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
		TScopeCounter<uint32> IterationLock(WriteLock);

		for (auto&& Token : Tokens)
		{
			if (IsValid(Token) && Token->IsA<TFaerieItemToken>())
			{
				if (!Iter(Cast<TFaerieItemToken>(Token)))
				{
					return;
				}
			}
		}
	}

	// Creates a new faerie item object with no tokens. These are instance-mutable by default.
	static UFaerieItem* CreateEmptyInstance(EFaerieItemMutabilityFlags Flags = EFaerieItemMutabilityFlags::None);

	// Creates a new faerie item object using this instance as a template. Instance-mutable only if required by item or flags.
	UFaerieItem* CreateInstance(EFaerieItemMutabilityFlags Flags = EFaerieItemMutabilityFlags::None) const;

	// Creates a new faerie item object using this instance as a template. Duplicates are instance-mutable by default.
	UFaerieItem* CreateDuplicate(EFaerieItemMutabilityFlags Flags = EFaerieItemMutabilityFlags::None) const;

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

protected:
	// @todo this isn't const safe
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem", meta = (DeterminesOutputType = Class, DynamicOutputParam = FoundToken, ExpandBoolAsExecs = ReturnValue))
	bool FindToken(TSubclassOf<UFaerieItemToken> Class, UFaerieItemToken*& FoundToken) const;

	// @todo this isn't const safe
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem", meta = (DeterminesOutputType = Class, DynamicOutputParam = FoundTokens))
	void FindTokens(TSubclassOf<UFaerieItemToken> Class, TArray<UFaerieItemToken*>& FoundTokens) const;

public:
	UFUNCTION(BlueprintCallable, Category = "FaerieItem", BlueprintAuthorityOnly)
	void AddToken(UFaerieItemToken* Token);

	UFUNCTION(BlueprintCallable, Category = "FaerieItem", BlueprintAuthorityOnly)
	bool RemoveToken(UFaerieItemToken* Token);

	UFUNCTION(BlueprintCallable, Category = "FaerieItem", BlueprintAuthorityOnly)
	int32 RemoveTokensByClass(TSubclassOf<UFaerieItemToken> Class);

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
	FNotifyOwnerOfSelfMutation::RegistrationType& GetNotifyOwnerOfSelfMutation() { return NotifyOwnerOfSelfMutation; }

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
	FNotifyOwnerOfSelfMutation NotifyOwnerOfSelfMutation;

	// Is writing to Tokens locked?
	mutable uint32 WriteLock = 0;
};