// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItem.h"
#include "FaerieHashStatics.h"
#include "FaerieItemToken.h"
#include "AssetLoadFlagFixer.h"
#include "Algo/Copy.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItem)

DEFINE_LOG_CATEGORY(LogFaerieItem)

namespace Faerie::Tags
{
	UE_DEFINE_GAMEPLAY_TAG(TokenAdd, "Fae.Token.Add")
	UE_DEFINE_GAMEPLAY_TAG(TokenRemove, "Fae.Token.Remove")
	UE_DEFINE_GAMEPLAY_TAG(TokenGenericPropertyEdit, "Fae.Token.GenericPropertyEdit")
}

#if WITH_EDITOR
// This is really the module startup time, since this is set whenever this module loads :)
static FDateTime EditorStartupTime = FDateTime::UtcNow();
#endif

namespace Faerie
{
	FTokenFilter& FTokenFilter::ByClass(const TSubclassOf<UFaerieItemToken>& Class)
	{
		if (!IsValid(Class) ||
			Class == UFaerieItemToken::StaticClass()) return *this;

		Tokens.RemoveAllSwap(
			[Class](const TObjectPtr<UFaerieItemToken>& Token)
			{
				return !Token.IsA(Class);
			});

		return *this;
	}

	FTokenFilter& FTokenFilter::ByTag(const FGameplayTag& Tag, const bool Exact)
	{
		Tokens.RemoveAllSwap(
			[&](const TObjectPtr<UFaerieItemToken>& Token)
			{
				if (Exact)
				{
					return !Token->GetClassTags().HasTagExact(Tag);
				}
				return !Token->GetClassTags().HasTag(Tag);
			});
		return *this;
	}

	FTokenFilter& FTokenFilter::ByTags(const FGameplayTagContainer& Tags, const bool All, const bool Exact)
	{
		Tokens.RemoveAllSwap(
			[&](const TObjectPtr<UFaerieItemToken>& Token)
			{
				if (Exact)
				{
					if (All)
					{
						return !Token->GetClassTags().HasAllExact(Tags);
					}
					return !Token->GetClassTags().HasAnyExact(Tags);
				}

				if (All)
				{
					return !Token->GetClassTags().HasAll(Tags);
				}
				return !Token->GetClassTags().HasAny(Tags);
			});
		return *this;
	}

	FTokenFilter& FTokenFilter::ByTagQuery(const FGameplayTagQuery& Query)
	{
		Tokens.RemoveAllSwap(
			[&](const TObjectPtr<UFaerieItemToken>& Token)
			{
				return !Token->GetClassTags().MatchesQuery(Query);
			});
		return *this;
	}

	FTokenFilter& FTokenFilter::ForEach(TBreakableLoop<const TObjectPtr<UFaerieItemToken>&> Iter)
	{
		for (auto&& Token : Tokens)
		{
			if (IsValid(Token))
			{
				if (Iter(Token) == Stop)
				{
					return *this;
				}
			}
		}

		return *this;
	}
}

using namespace Faerie;

void UFaerieItem::PreSave(FObjectPreSaveContext SaveContext)
{
	CacheTokenMutability();

#if WITH_EDITOR
	// This is a random hoot I'm adding to be funny. The LastModified timestamp only really matters for mutable items,
	// so for any other random item in the game, this time would be meaningless. So instead I'm going to encode the time
	// that the item was last saved in-editor. For items visible in-editor this is will tell the dev when the item was
	// last touched. Since PreSave is called on all assets during packaging, theoretically, assets in a packaged build
	// will all have the same timestamp.
	LastModified = EditorStartupTime;
#endif

	Super::PreSave(SaveContext);
}

void UFaerieItem::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	// Items loaded from disk in shipping builds don't need to re-cache this.
	CacheTokenMutability();
#endif
}

void UFaerieItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// For Tokens & LastModified, they only need to replicate past the initial bunch if they are mutable.
	SharedParams.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Tokens, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LastModified, SharedParams);

	// Mutability doesn't change after the initial token setup, so it only needs to be sent on the first rep.
	SharedParams.Condition = COND_InitialOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MutabilityFlags, SharedParams);
}


void UFaerieItem::GetReplicatedCustomConditionState(FCustomPropertyConditionState& OutActiveState) const
{
	Super::GetReplicatedCustomConditionState(OutActiveState);

	if (IsDataMutable())
	{
		// Replicate mutable data unconditionally.
		DOREPDYNAMICCONDITION_INITCONDITION_FAST(ThisClass, Tokens, COND_None);
		DOREPDYNAMICCONDITION_INITCONDITION_FAST(ThisClass, LastModified, COND_None);
	}
	else
	{
		// Replicate immutable data only in the initial bunch.
		DOREPDYNAMICCONDITION_INITCONDITION_FAST(ThisClass, Tokens, COND_InitialOnly);
		DOREPDYNAMICCONDITION_INITCONDITION_FAST(ThisClass, LastModified, COND_InitialOnly);
	}
}

void UFaerieItem::ForEachToken(TBreakableLoop<const TObjectPtr<UFaerieItemToken>&> Iter)
{
	TScopeCounter<uint32> IterationLock(WriteLock);

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token))
		{
			if (Iter(Token) == Stop)
			{
				return;
			}
		}
	}
}

void UFaerieItem::ForEachToken(TBreakableLoop<const TObjectPtr<const UFaerieItemToken>&> Iter) const
{
	TScopeCounter<uint32> IterationLock(WriteLock);

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token))
		{
			if (Iter(Token) == Stop)
			{
				return;
			}
		}
	}
}

void UFaerieItem::ForEachTokenOfClass(TBreakableLoop<const TObjectPtr<UFaerieItemToken>&> Iter, const TSubclassOf<UFaerieItemToken>& Class)
{
	TScopeCounter<uint32> IterationLock(WriteLock);

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token) && Token->IsA(Class))
		{
			if (Iter(Token) == Stop)
			{
				return;
			}
		}
	}
}

void UFaerieItem::ForEachTokenOfClass(TBreakableLoop<const TObjectPtr<const UFaerieItemToken>&> Iter, const TSubclassOf<UFaerieItemToken>& Class) const
{
	TScopeCounter<uint32> IterationLock(WriteLock);

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token) && Token->IsA(Class))
		{
			if (Iter(Token) == Stop)
			{
				return;
			}
		}
	}
}

UFaerieItem* UFaerieItem::CreateNewInstance(const TConstArrayView<UFaerieItemToken*> Tokens, const EFaerieItemInstancingMutability Mutability)
{
	UFaerieItem* Instance = NewObject<UFaerieItem>();
	EnumAddFlags(Instance->MutabilityFlags, ToFlags(Mutability) | EFaerieItemMutabilityFlags::InstanceMutability);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MutabilityFlags, Instance);

	for (auto&& Token : Tokens)
	{
		// Mutable tokens have to be owned by us.
		if (Token->IsMutable())
		{
			check(Token->GetOuter() == GetTransientPackage())
			Token->Rename(nullptr, Instance);
		}
		else
		{
			if (Token->GetOuter() == GetTransientPackage())
			{
				Token->Rename(nullptr, Instance);
			}
		}

		Instance->Tokens.Add(Token);
	}
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Tokens, Instance);

	Instance->LastModified = FDateTime::UtcNow();
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastModified, Instance);

	return Instance;
}

UFaerieItem* UFaerieItem::CreateInstance(const EFaerieItemInstancingMutability Mutability) const
{
	const bool ShouldCreateDuplicate = [this, Mutability]
	{
		if (EnumHasAnyFlags(Mutability, EFaerieItemInstancingMutability::Immutable))
		{
			return false;
		}
		if (EnumHasAnyFlags(Mutability, EFaerieItemInstancingMutability::Mutable))
		{
			return true;
		}

		// Default to cached state.
		return IsDataMutable();
	}();

	UFaerieItem* NewInstance;

	if (ShouldCreateDuplicate)
	{
		// Make a copy of the static item stored in this asset if we might need to modify the data
		NewInstance = CreateDuplicate(Mutability);
	}
	else
	{
		// If the item is not mutable, we can just reference the single copy of it.
		// @todo instead of const_cast, return const safe struct wrapper.
		NewInstance = const_cast<ThisClass*>(this);
	}

	return NewInstance;
}

UFaerieItem* UFaerieItem::CreateDuplicate(const EFaerieItemInstancingMutability Mutability) const
{
	UFaerieItem* Duplicate = NewObject<UFaerieItem>();
	EnumAddFlags(Duplicate->MutabilityFlags, ToFlags(Mutability) | EFaerieItemMutabilityFlags::InstanceMutability);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MutabilityFlags, Duplicate);

	// Add our tokens to the new object.
	ForEachToken(
		[Duplicate](const TObjectPtr<const UFaerieItemToken>& Token)
		{
			// Mutable tokens must be duplicated.
			if (Token->IsMutable())
			{
				Duplicate->Tokens.Add(DuplicateObjectFromDiskForReplication(Token, Duplicate));
			}
			// Immutable tokens can be referenced from the asset directly.
			else
			{
				Duplicate->Tokens.Add(ConstCast(Token));
			}
			return Continue;
		});

	// Initialize token mutability.
	Duplicate->CacheTokenMutability();

	Duplicate->LastModified = FDateTime::UtcNow();

	return Duplicate;
}

const UFaerieItemToken* UFaerieItem::GetToken(const TSubclassOf<UFaerieItemToken>& Class) const
{
	if (!ensure(IsValid(Class)))
	{
		return nullptr;
	}

	if (!ensure(Class != UFaerieItemToken::StaticClass()))
	{
		return nullptr;
	}

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token) && Token.IsA(Class))
		{
			return Token;
		}
	}

	return nullptr;
}

TArray<const UFaerieItemToken*> UFaerieItem::GetTokens(const TSubclassOf<UFaerieItemToken>& Class) const
{
	if (!ensure(IsValid(Class)))
	{
		return {};
	}

	if (!ensure(Class != UFaerieItemToken::StaticClass()))
	{
		return {};
	}

	TArray<const UFaerieItemToken*> OutTokens;

	Algo::CopyIf(Tokens, OutTokens,
		[Class](const UFaerieItemToken* Token)
		{
			return IsValid(Token) && Token->IsA(Class);
		});

	return OutTokens;
}

UFaerieItemToken* UFaerieItem::GetMutableToken(const TSubclassOf<UFaerieItemToken>& Class)
{
	if (!ensure(IsValid(Class)) ||
		!ensure(Class != UFaerieItemToken::StaticClass()) ||
		!IsDataMutable())
	{
		return {};
	}

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token) && Token.IsA(Class))
		{
			return Token;
		}
	}

	return nullptr;
}

TArray<UFaerieItemToken*> UFaerieItem::GetMutableTokens(const TSubclassOf<UFaerieItemToken>& Class)
{
	if (!ensure(IsValid(Class)) ||
		!ensure(Class != UFaerieItemToken::StaticClass()) ||
		!IsDataMutable())
	{
		return {};
	}

	TArray<UFaerieItemToken*> OutTokens;

	Algo::CopyIf(Tokens, OutTokens,
		[Class](const UFaerieItemToken* Token)
		{
			return IsValid(Token) && Token->IsA(Class);
		});

	return OutTokens;
}

FTokenFilter UFaerieItem::FilterTokens() const
{
	return FTokenFilter(Tokens);
}

bool UFaerieItem::Compare(const UFaerieItem* A, const UFaerieItem* B, const EFaerieItemComparisonFlags Flags)
{
	if (!A || !B) return A == B;
	return A->CompareWith(B, Flags);
}

bool UFaerieItem::CompareWith(const UFaerieItem* Other, const EFaerieItemComparisonFlags Flags) const
{
	// If we are the same object, then we already know we're identical
	if (this == Other)
	{
		return true;
	}

	// Mutability comparison
	if (!EnumHasAnyFlags(Flags, EFaerieItemComparisonFlags::Mutability_Ignore))
	{
		if (EnumHasAnyFlags(Flags, EFaerieItemComparisonFlags::Mutability_TreatAsUnequivocable))
		{
			// If either is mutable, then they are considered "unequivocable" and therefor, mutually exclusive.
			if (IsDataMutable() || Other->IsDataMutable())
			{
				return false;
			}
		}
		else if (EnumHasAnyFlags(Flags, EFaerieItemComparisonFlags::Mutability_Compare))
		{
			// If mutability doesn't match, then the items fail comparison.
			if (IsDataMutable() != Other->IsDataMutable())
			{
				return false;
			}
		}
	}

	// Compare primary identifiers.
	// This is a quicker comparison that uses the CompareWith virtual function implemented by those tokens.
	if (EnumHasAnyFlags(Flags, EFaerieItemComparisonFlags::Tokens_ComparePrimaryIdentifiers))
	{
		FTokenFilter ThisFilter(FilterTokens());
		ThisFilter.ByTag(Tags::PrimaryIdentifierToken);

		FTokenFilter OtherFilter(Other->FilterTokens());
		OtherFilter.ByTag(Tags::PrimaryIdentifierToken);

		// This already indicates they are not equal.
		if (ThisFilter.Num() != OtherFilter.Num())
		{
			return false;
		}

		bool ComparisonFailed = false;

		OtherFilter.ForEach([&ThisFilter, &ComparisonFailed](const TObjectPtr<UFaerieItemToken>& TokenB)
			{
				// Each token from Other must find a token in ItemAPrimaries that it compares to.
				for (auto It =
					reinterpret_cast<TArray<TObjectPtr<UFaerieItemToken>>*>(&ThisFilter) // This is a hack to allow us to use RemoveCurrentSwap
					->CreateIterator(); It; ++It)
				{
					if (TokenB->CompareWith(*It))
					{
						// The token is a match! Remove from the ItemA list, and continue to the next token.
						It.RemoveCurrentSwap();
						return Continue;
					}
				}

				// No match was found, marked comparison as failed, and exit.
				ComparisonFailed = true;
				return Stop;
			});

		return !ComparisonFailed;
	}

	// Resort to comparing all tokens. Since most tokens don't implement CompareWith, fallback on hashing the objects.
	TArray<TObjectPtr<UFaerieItemToken>> TokensA = Tokens;
	TArray<TObjectPtr<UFaerieItemToken>> TokensB = Other->Tokens;

	// This already indicates they are not equal.
	if (TokensA.Num() != TokensB.Num())
	{
		return false;
	}

	// First pass is to remove direct pointer copies (asset referenced tokens)
	for (auto ItA(TokensA.CreateIterator()); ItA; ++ItA)
	{
		for (auto ItB(TokensB.CreateIterator()); ItB; ++ItB)
		{
			if (*ItA == *ItB)
			{
				ItA.RemoveCurrentSwap();
				ItB.RemoveCurrentSwap();
				break;
			}
		}
	}

	// Hash all of A's tokens
	TArray<uint32> TokenAHashes;
	TokenAHashes.Reserve(TokensA.Num());
	for (auto&& TokenA : TokensA)
	{
		TokenAHashes.Add(Hash::HashObjectByProps(TokenA, true));
	}

	// While hashing all of B's tokens, check that they all have a match.
	for (auto&& TokenB : TokensB)
	{
		if (const uint32 TokenBHash = Hash::HashObjectByProps(TokenB, true);
			TokenAHashes.RemoveSwap(TokenBHash))
		{
			// The token was matched! Continue iteration.
			continue;
		}

		// No match was found, exit.
		return false;
	}

	// They are equal then :)
	return true;
}

UFaerieItem* UFaerieItem::MutateCast() const
{
	if (CanMutate())
	{
		return const_cast<UFaerieItem*>(this);
	}
	return nullptr;
}

bool UFaerieItem::AddToken(UFaerieItemToken* Token)
{
	if (!ensure(IsValid(Token)))
	{
		return false;
	}

	if (!ensure(CanMutate()))
	{
		return false;
	}

	if (!ensure(WriteLock == 0))
	{
		UE_LOG(LogFaerieItem, Error, TEXT("Cannot AddToken while iterating in ForEachToken. Please correct code."))
		return false;
	}

	if (Token->GetOuter() == GetTransientPackage())
	{
		// Newly created Tokens need to be outer'd to this item.
		Token->Rename(nullptr, this);
	}
	else
	{
		// If this check fails, then whatever code tried to create the token used an outer other than us, and needs to
		// either duplicate or rename the token with us as the outer.
		check(Token->GetOuter() == this);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastModified, this);
	LastModified = FDateTime::UtcNow();

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Tokens, this);
	Tokens.Add(Token);

	(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Tags::TokenAdd);
	return true;
}

bool UFaerieItem::RemoveToken(UFaerieItemToken* Token)
{
	if (!ensure(IsValid(Token)))
	{
		return false;
	}

	if (!ensure(CanMutate()))
	{
		return false;
	}

	if (!ensure(WriteLock == 0))
	{
		UE_LOG(LogFaerieItem, Error, TEXT("Cannot AddToken while iterating in ForEachToken. Please correct code."))
		return false;
	}

	if (!!Tokens.Remove(Token))
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Tokens, this);

		LastModified = FDateTime::UtcNow();
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastModified, this);

		(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Tags::TokenRemove);

		return true;
	}

	return false;
}

int32 UFaerieItem::RemoveTokensByClass(const TSubclassOf<UFaerieItemToken> Class)
{
	if (!ensure(IsValid(Class)))
	{
		return 0;
	}

	if (!ensure(Class != UFaerieItemToken::StaticClass()))
	{
		return 0;
	}

	if (!ensure(CanMutate()))
	{
		return 0;
	}

	if (!ensure(WriteLock == 0))
	{
		UE_LOG(LogFaerieItem, Error, TEXT("Cannot AddToken while iterating in ForEachToken. Please correct code."))
		return false;
	}

	TArray<const UFaerieItemToken*> TokensRemoved;

	if (const int32 Removed = Tokens.RemoveAllSwap(
		[Class, this, &TokensRemoved](const UFaerieItemToken* Token)
		{
			const bool Removing = IsValid(Token) && Token->GetClass() == Class;
			if (Removing)
			{
				TokensRemoved.Add(Token);
			}
			return Removing;
		}))
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Tokens, this);

		LastModified = FDateTime::UtcNow();
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastModified, this);

		for (auto&& Token : TokensRemoved)
		{
			(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Tags::TokenRemove);
		}

		return Removed;
	}

	return 0;
}

TArray<UFaerieItemToken*> UFaerieItem::GetAllTokens() const
{
	return Tokens;
}

bool UFaerieItem::FindToken(const TSubclassOf<UFaerieItemToken> Class, UFaerieItemToken*& FoundToken) const
{
	// @Note: BP doesn't understand const-ness, but since UFaerieItemToken does not have a BP accessible API that can
	// mutate it, it's perfectly safe.
	FoundToken = const_cast<UFaerieItemToken*>(GetToken(Class));
	return FoundToken != nullptr;
}

void UFaerieItem::FindTokens(const TSubclassOf<UFaerieItemToken> Class, TArray<UFaerieItemToken*>& FoundTokens) const
{
	// Can't use GetMutableTokens here because it'd fail to return anything if *this* is not data mutable as a precaution.
	// @Note: BP doesn't understand const-ness, but since UFaerieItemToken does not have a BP accessible API that can
	// mutate it, it's perfectly safe.
	FoundTokens = Type::Cast<TArray<UFaerieItemToken*>>(GetTokens(Class));
}

TArray<UFaerieItemToken*> UFaerieItem::FindTokensByTag(const FGameplayTag& Tag, const bool Exact) const
{
	return FilterTokens().ByTag(Tag, Exact).BlueprintOnlyAccess();
}

TArray<UFaerieItemToken*> UFaerieItem::FindTokensByTags(const FGameplayTagContainer& Tags, const bool All,
	const bool Exact) const
{
	return FilterTokens().ByTags(Tags, All, Exact).BlueprintOnlyAccess();
}

TArray<UFaerieItemToken*> UFaerieItem::FindTokensByTagQuery(const FGameplayTagQuery& Query) const
{
	return FilterTokens().ByTagQuery(Query).BlueprintOnlyAccess();
}

/*
EFaerieItemSourceType UFaerieItem::GetSourceType() const
{
	if (GetTypedOuter())
	{
		// @todo how to determine Dynamic / Asset outer
	}
	if (GetOuter() == GetTransientPackage())
	{
		// We are a dynamic that is currently unowned.
		return EFaerieItemSourceType::Dynamic;
	}

	return EFaerieItemSourceType::Asset;
}
*/

bool UFaerieItem::IsInstanceMutable() const
{
	return EnumHasAllFlags(MutabilityFlags, EFaerieItemMutabilityFlags::InstanceMutability);
}

bool UFaerieItem::IsDataMutable() const
{
	return EnumHasAllFlags(MutabilityFlags, EFaerieItemMutabilityFlags::TokenMutability);
}

bool UFaerieItem::CanMutate() const
{
	return EnumHasAllFlags(MutabilityFlags, EFaerieItemMutabilityFlags::InstanceMutability | EFaerieItemMutabilityFlags::TokenMutability);
}

void UFaerieItem::OnTokenEdited(const UFaerieItemToken* Token)
{
	check(CanMutate())
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastModified, this);
	LastModified = FDateTime::UtcNow();
	(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Tags::TokenGenericPropertyEdit);
}

void UFaerieItem::CacheTokenMutability()
{
	const bool DeterminedToBeMutable = [this]
	{
		if (EnumHasAnyFlags(MutabilityFlags, EFaerieItemMutabilityFlags::ForbidTokenMutability))
		{
			return false;
		}

		if (EnumHasAnyFlags(MutabilityFlags, EFaerieItemMutabilityFlags::AlwaysTokenMutable))
		{
			return true;
		}

		// If any token has mutable data, mark this item with the TokenMutability flag.
		for (auto&& Token : Tokens)
		{
			if (IsValid(Token) && Token->IsMutable())
			{
				return true;
			}
		}

		// No token needs mutability, so without a flag to say otherwise, assume mutability as false.
		return false;
	}();

	if (DeterminedToBeMutable)
	{
		// If determined to be true, mark this item with the TokenMutability flag.
		if (!EnumHasAnyFlags(MutabilityFlags, EFaerieItemMutabilityFlags::TokenMutability))
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MutabilityFlags, this);
			EnumAddFlags(MutabilityFlags, EFaerieItemMutabilityFlags::TokenMutability);
			(void)MarkPackageDirty();
		}
	}
	else
	{
		// Otherwise, make sure we *don't* have that flag.
		if (EnumHasAnyFlags(MutabilityFlags, EFaerieItemMutabilityFlags::TokenMutability))
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MutabilityFlags, this);
			EnumRemoveFlags(MutabilityFlags, EFaerieItemMutabilityFlags::TokenMutability);
			(void)MarkPackageDirty();
		}
	}
}