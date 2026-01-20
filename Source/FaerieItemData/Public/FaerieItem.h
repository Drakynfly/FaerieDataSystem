// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "FaerieItemDataEnums.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "NetSupportedObject.h"
#include "Templates/SubclassOf.h"

#include "FaerieItem.generated.h"

class UFaerieItem;
class UFaerieItemToken;

namespace Faerie
{
	namespace Token::Private
	{
		class FIteratorAccess;
	}

	namespace Tags
	{
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenAdd)
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenRemove)
		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenGenericPropertyEdit)

		FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenReferenceDefaults)
	}

	using FNotifyOwnerOfSelfMutation = TDelegate<void(const UFaerieItem*, const UFaerieItemToken*, FGameplayTag)>;
}

/**
 * A runtime instance of an item.
 */
UCLASS(DefaultToInstanced, EditInlineNew, BlueprintType)
class FAERIEITEMDATA_API UFaerieItem : public UNetSupportedObject
{
	GENERATED_BODY()

	friend UFaerieItemToken;
	friend class UFaerieItemAsset;
	friend Faerie::Token::Private::FIteratorAccess;

public:
	//~ Begin UObject interface
	virtual void PostInitProperties() override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void PostLoad() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void GetReplicatedCustomConditionState(FCustomPropertyConditionState& OutActiveState) const override;
	//~ Emd UObject interface

private:
	const UFaerieItemToken* GetTokenImpl(const TSubclassOf<UFaerieItemToken>& ValidatedClass, FGameplayTag ReferenceTag = Faerie::Tags::TokenReferenceDefaults) const;
	const UFaerieItemToken* GetOwnedTokenImpl(const TSubclassOf<UFaerieItemToken>& ValidatedClass) const;
	UFaerieItemToken* GetMutableTokenImpl(const TSubclassOf<UFaerieItemToken>& ValidatedClass);

public:
	// Creates a new faerie item object with the given tokens. These are instance-mutable by default.
	static UFaerieItem* CreateNewInstance(TConstArrayView<UFaerieItemToken*> Tokens, EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic);

	// Creates a new faerie item object using this instance as a template. Instance-mutable only if required by item or flags.
	UFaerieItem* CreateInstance(EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic) const;

	// Creates a new faerie item object using this instance as a template. Duplicates are instance-mutable by default.
	UFaerieItem* CreateDuplicate(EFaerieItemInstancingMutability Mutability = EFaerieItemInstancingMutability::Automatic) const;

	// Gets a view of all owned tokens in this item.
	TConstArrayView<TObjectPtr<UFaerieItemToken>> GetOwnedTokens() const { return Tokens; }

	// Gets the token at a specified index. Low-level access for when you know what you are doing.
	const UFaerieItemToken* GetTokenAtIndex(int32 Index) const;

	// Gets the first token of the specified class that is either owned or referenced.
	const UFaerieItemToken* GetToken(const TSubclassOf<UFaerieItemToken>& Class, FGameplayTag ReferenceTag = Faerie::Tags::TokenReferenceDefaults) const;

	// Gets the first token of the specified class that is either owned or referenced.
	template <Faerie::CItemTokenImpl T>
	const T* GetToken(FGameplayTag ReferenceTag = Faerie::Tags::TokenReferenceDefaults) const
	{
		return Cast<T>(GetTokenImpl(T::StaticClass(), ReferenceTag));
	}

	// Gets the first owned token of the specified class.
	const UFaerieItemToken* GetOwnedToken(const TSubclassOf<UFaerieItemToken>& Class) const;

	// Gets the first owned token of the specified class. Templated version.
	template <Faerie::CItemTokenImpl T>
	const T* GetOwnedToken() const
	{
		return Cast<T>(GetOwnedTokenImpl(T::StaticClass()));
	}

	// Gets mutable access to an owned token, if this item allows mutation.
	UFaerieItemToken* GetMutableToken(const TSubclassOf<UFaerieItemToken>& Class);

	// Gets mutable access to an owned token, if this item allows mutation. Templated version.
	template <Faerie::CItemTokenImpl T>
	T* GetMutableToken()
	{
		return Cast<T>(GetMutableTokenImpl(T::StaticClass()));
	}

	/*
	 * Compares two Items to determine if they are "the same". There are some limitations here imposed by the design of
	 * FDS. See EFaerieItemComparisonFlags for descriptions of the Flags.
	 */
	static bool Compare(const UFaerieItem* A, const UFaerieItem* B, const EFaerieItemComparisonFlags Flags);
	bool CompareWith(const UFaerieItem* Other, const EFaerieItemComparisonFlags Flags) const;


	//~		C++ Item Mutation		~//

	// Mutate cast will return a const_cast'd *this* if the item is a runtime mutable instance. This is the proscribed
	// method to gain access to the non-const API of UFaerieItem.
	[[nodiscard]] UFaerieItem* MutateCast() const;
	bool AddToken(UFaerieItemToken* Token);
	bool RemoveToken(const UFaerieItemToken* Token);
	bool ReplaceToken(const UFaerieItemToken* Old, UFaerieItemToken* New);
	int32 RemoveTokensByClass(TSubclassOf<UFaerieItemToken> Class);

protected:
	// While not returning const pointers, these are considered safe, as all mutating functions are locked being FFaerieItemEditHandle

	// Gets all tokens owned by this item.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem")
	TArray<UFaerieItemToken*> GetAllTokens() const;

	// Gets the first token of the given class
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "FaerieItem", meta = (DeterminesOutputType = Class, DynamicOutputParam = FoundToken, ExpandBoolAsExecs = ReturnValue))
	bool FindToken(TSubclassOf<UFaerieItemToken> Class, UFaerieItemToken*& FoundToken) const;

public:
	UFUNCTION(BlueprintCallable, Category = "FaerieItem")
	FDateTime GetLastModified() const { return LastModified; }

	// Can this item object be changed whatsoever at runtime? This is not available for asset-referenced or precached items.
	UFUNCTION(BlueprintCallable, Category = "FaerieItem")
	bool IsInstanceMutable() const;

	// Are any tokens in this item capable of being changed at runtime?
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

private:
	// Delegate for owners to bind to, for detecting when tokens are mutated outside their knowledge
	Faerie::FNotifyOwnerOfSelfMutation NotifyOwnerOfSelfMutation;

protected:
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FaerieItem")
	TArray<TObjectPtr<UFaerieItemToken>> Tokens;

	// Keeps track of the last time this item was modified. Allows, for example, sorting items by recently touched.
	// Only intended to be useful for mutable and dynamically-generated items. Asset-derived instances will be set
	// to the time that they were cooked (for shipping) or last edited (at dev-time).
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FaerieItem")
	FDateTime LastModified = FDateTime();

	// Mutability flags.
	// In order for an item instance to be changed at runtime, it must have no mutually exclusive flags.
	// This is only set once at item creation, and cannot change after.
	UPROPERTY(Replicated, VisibleInstanceOnly, Category = "FaerieItem")
	EFaerieItemMutabilityFlags MutabilityFlags;

private:
	// Is writing to Tokens locked?
	mutable uint32 WriteLock = 0;

protected:
	UE_DEPRECATED(5.6, TEXT("Replaced by UFaerieItemDataLibrary::FindTokensByClass"))
	UFUNCTION(BlueprintCallable, BlueprintPure = false, meta = (DeterminesOutputType = Class, DynamicOutputParam = FoundTokens, deprecated, DeprecationMessage = "Replaced by UFaerieItemDataLibrary::FindTokensByClass"))
	void FindTokens(TSubclassOf<UFaerieItemToken> Class, TArray<UFaerieItemToken*>& FoundTokens) const;
};