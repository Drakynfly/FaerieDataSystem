// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItem.h"
#include "FaerieItemToken.h"
#include "AssetLoadFlagFixer.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemTokenFilter.h"
#include "FaerieItemTokenFilterTypes.h"
#include "Algo/Copy.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItem)

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

using namespace Faerie;

void UFaerieItem::PostInitProperties()
{
	Super::PostInitProperties();

	CacheTokenMutability();
}

void UFaerieItem::PreSave(FObjectPreSaveContext SaveContext)
{
	CacheTokenMutability();

#if WITH_EDITOR
	// This is a random hoot I'm adding to be funny. The LastModified timestamp only really matters for mutable items,
	// so for any other random item in the game, this time would be meaningless. So instead I'm going to encode the time
	// that the item was last saved in-editor. For items visible in-editor this will tell the dev when the item was
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
		// Immutable tokens only get outer'd to us if they are currently transient, likely newly created.
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

	// Initialize token mutability.
	Instance->CacheTokenMutability();

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
	Duplicate->Tokens.Reserve(Tokens.Num());
	for (const TObjectPtr<UFaerieItemToken>& Token : Tokens)
	{
		// Mutable tokens must be duplicated.
		if (Token->IsMutable())
		{
			Duplicate->Tokens.Add(DuplicateObjectFromDiskForReplication(Token.Get(), Duplicate));
		}
		// Immutable tokens can be referenced from the asset directly.
		else
		{
			Duplicate->Tokens.Add(ConstCast(Token));
		}
	}

	// Initialize token mutability.
	Duplicate->CacheTokenMutability();

	Duplicate->LastModified = FDateTime::UtcNow();

	return Duplicate;
}

const UFaerieItemToken* UFaerieItem::GetTokenAtIndex(const int32 Index) const
{
	return Tokens[Index];
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
		if (IsValid(Token) &&
			Token.IsA(Class) &&
			Token->IsMutable())
		{
			return Token;
		}
	}

	return nullptr;
}

bool UFaerieItem::Compare(const UFaerieItem* A, const UFaerieItem* B, const EFaerieItemComparisonFlags Flags)
{
	if (!A || !B) return A == B;
	return A->CompareWith(B, Flags);
}

bool UFaerieItem::CompareWith(const UFaerieItem* Other, const EFaerieItemComparisonFlags Flags) const
{
	QUICK_SCOPE_CYCLE_COUNTER(UFaerieItem_CompareWith);

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
		using namespace Faerie::Token;
		return Filter(this).By<FTagFilter>(Tags::PrimaryIdentifierToken)
			.CompareTokens(Filter(Other).By<FTagFilter>(Tags::PrimaryIdentifierToken));
	}

	// This already indicates they are not equal.
	if (Tokens.Num() != Other->Tokens.Num())
	{
		return false;
	}

	// Resort to comparing all tokens. Since most tokens don't implement CompareWith, fallback on hashing the objects.
	TArray<TObjectPtr<UFaerieItemToken>> TokensA = Tokens;
	TArray<TObjectPtr<UFaerieItemToken>> TokensB = Other->Tokens;

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
		TokenAHashes.Add(TokenA->GetTokenHash());
	}

	// While hashing all of B's tokens, check that they all have a match.
	for (auto&& TokenB : TokensB)
	{
		if (const uint32 TokenBHash = TokenB->GetTokenHash();
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
		UE_LOG(LogFaerieItemData, Error, TEXT("Cannot AddToken while iterating Tokens. Please correct code."))
		return false;
	}

	// Mutable tokens have to be owned by us.
	if (Token->IsMutable())
	{
		// If this check fails, then whatever code tried to create the token used an outer other than us, and needs to
		// either duplicate or rename the token with us as the outer.
		check(Token->GetOuter() == GetTransientPackage() || Token->GetOuter() == this)
		Token->Rename(nullptr, this);
	}
	// Immutable tokens only get outer'd to us if they are currently transient, likely newly created.
	else
	{
		if (Token->GetOuter() == GetTransientPackage())
		{
			Token->Rename(nullptr, this);
		}
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
		UE_LOG(LogFaerieItemData, Error, TEXT("Cannot RemoveToken while iterating Tokens. Please correct code."))
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
		UE_LOG(LogFaerieItemData, Error, TEXT("Cannot RemoveToken while iterating Tokens. Please correct code."))
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

#include "Libraries/FaerieItemDataLibrary.h"

void UFaerieItem::FindTokens(const TSubclassOf<UFaerieItemToken> Class, TArray<UFaerieItemToken*>& FoundTokens) const
{
	UFaerieItemDataLibrary::FindTokensByClass(this, Class, FoundTokens);
}