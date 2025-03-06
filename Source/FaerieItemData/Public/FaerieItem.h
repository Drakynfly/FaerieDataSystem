// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "TypeCastingUtils.h"

#include "FaerieItem.generated.h"

UENUM(Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFaerieItemMutabilityFlags : uint8
{
	None = 0,

	// 'Instance Mutability' is a required flag for item instances to be changed after construction. Without this flag,
	// items are understood to be forever immutable because it either lives in a package, or other outer that is static
	// and itself cannot be changed, regardless of whether the instance has Token Mutability enabled.
	// Items created at runtime or duplicated from static instances have this flag enabled by default.
	// (See the definitions of CreateInstance / CreateDuplicate)
	InstanceMutability = 1 << 0,

	// 'Token Mutability' says that this item has one or more tokens that request the ability to mutate their internal state.
	// Mutability at runtime is only allowed if InstanceMutability is also enabled.
	TokenMutability = 1 << 1,

	// Enable to make all instances of this item mutable, even if no current Tokens request mutability. This is usually
	// required when making an item template expected to have a mutable token added dynamically at runtime, but
	// doesn't have any mutable tokens added by the editor.
	AlwaysTokenMutable = 1 << 2,

	// Prevent token mutation at runtime, even if TokenMutability is enabled.
	ForbidTokenMutability = 1 << 3,
};
ENUM_CLASS_FLAGS(EFaerieItemMutabilityFlags)

/*
UENUM(BlueprintType)
enum class EFaerieItemSourceType : uint8
{
	// This item instance is a reference to an asset that lives on disk. Replication is by name/netguid only, thus mutation is disallowed.
	// To add/remove/edit tokens, a duplicate of this item must be made. (UFaerieItem::CreateDuplicate)
	Asset,

	// This item instance was dynamically created at runtime, and lives only in memory. The entire object is replicated,
	// and token editing is enabled if TokenMutability is set on the MutabilityFlags (determined by source asset if ever allowable).
	Dynamic
};
*/

using FNotifyOwnerOfSelfMutation = TDelegate<void(const class UFaerieItem*, const class UFaerieItemToken*, FGameplayTag)>;

namespace Faerie::Tags
{
	FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenAdd)
	FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenRemove)
	FAERIEITEMDATA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TokenGenericPropertyEdit)
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

	static bool Compare(const UFaerieItem* A, const UFaerieItem* B);
	bool CompareWith(const UFaerieItem* Other) const;

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
};