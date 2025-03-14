// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItem.h"
#include "FaerieItemToken.h"
#include "FaerieUtils.h"
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

void UFaerieItem::ForEachToken(const TFunctionRef<bool(const TObjectPtr<UFaerieItemToken>&)>& Iter) const
{
	TScopeCounter<uint32> IterationLock(WriteLock);

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token))
		{
			if (!Iter(Token))
			{
				return;
			}
		}
	}
}

void UFaerieItem::ForEachTokenOfClass(const TFunctionRef<bool(const TObjectPtr<UFaerieItemToken>&)>& Iter, const TSubclassOf<UFaerieItemToken>& Class) const
{
	TScopeCounter<uint32> IterationLock(WriteLock);

	for (auto&& Token : Tokens)
	{
		if (IsValid(Token) && Token->IsA(Class))
		{
			if (!Iter(Token))
			{
				return;
			}
		}
	}
}

UFaerieItem* UFaerieItem::CreateEmptyInstance(const EFaerieItemMutabilityFlags Flags)
{
	UFaerieItem* Instance = NewObject<UFaerieItem>();
	EnumAddFlags(Instance->MutabilityFlags, Flags | EFaerieItemMutabilityFlags::InstanceMutability);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MutabilityFlags, Instance);
	Instance->LastModified = FDateTime::UtcNow();
	return Instance;
}

UFaerieItem* UFaerieItem::CreateInstance(const EFaerieItemMutabilityFlags Flags) const
{
	const bool ShouldCreateDuplicate = [this, Flags]
	{
		if (EnumHasAnyFlags(Flags, EFaerieItemMutabilityFlags::ForbidTokenMutability))
		{
			return false;
		}
		if (EnumHasAnyFlags(Flags, EFaerieItemMutabilityFlags::AlwaysTokenMutable))
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
		NewInstance = CreateDuplicate(Flags);
	}
	else
	{
		// If the item is not mutable, we can just reference the single copy of it.
		// @todo instead of const_cast, return const safe struct wrapper.
		NewInstance = const_cast<ThisClass*>(this);
	}

	return NewInstance;
}

UFaerieItem* UFaerieItem::CreateDuplicate(const EFaerieItemMutabilityFlags Flags) const
{
	UFaerieItem* Duplicate = NewObject<UFaerieItem>();
	EnumAddFlags(Duplicate->MutabilityFlags, Flags | EFaerieItemMutabilityFlags::InstanceMutability);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MutabilityFlags, Duplicate);

	// Add our tokens to the new object.
	ForEachToken(
		[Duplicate](const TObjectPtr<UFaerieItemToken>& Token)
		{
			// Mutable tokens must be duplicated.
			if (Token->IsMutable())
			{
				Duplicate->Tokens.Add(Faerie::DuplicateObjectFromDiskForReplication(Token, Duplicate));
			}
			// Immutable tokens can be referenced from the asset directly.
			else
			{
				Duplicate->Tokens.Add(Token);
			}
			return true;
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

bool UFaerieItem::Compare(const UFaerieItem* A, const UFaerieItem* B)
{
	if (!A || !B) return A == B;
	return A->CompareWith(B);
}

bool UFaerieItem::CompareWith(const UFaerieItem* Other) const
{
	// If we are the same object, then we already know we're identical
	if (this == Other)
	{
		return true;
	}

	// If either is mutable then they are considered "unequivocable" and therefor, mutually exclusive.
	if (IsDataMutable() || Other->IsDataMutable())
	{
		return false;
	}

	// Resort to comparing tokens ...
	const TConstArrayView<TObjectPtr<UFaerieItemToken>> TokensA = GetTokens();
	const TConstArrayView<TObjectPtr<UFaerieItemToken>> TokensB = Other->GetTokens();

	// This already indicates they are not equal.
	if (TokensA.Num() != TokensB.Num())
	{
		return false;
	}

	TMap<UClass*, TObjectPtr<UFaerieItemToken>> TokenMapA;

	// Get the classes of tokens in A
	for (auto&& Token : TokensA)
	{
		TokenMapA.Add(Token.GetClass(), Token);
	}

	TArray<TPair<TObjectPtr<UFaerieItemToken>, TObjectPtr<UFaerieItemToken>>> TokenPairs;

	// Check that for every token in A, there is one that matches class in B
	for (auto&& TokenB : TokensB)
	{
		auto&& TokenA = TokenMapA.Find(TokenB.GetClass());

		if (TokenA == nullptr)
		{
			return false;
		}

		TokenPairs.Add({*TokenA, TokenB});
	}

	for (auto&& [AToken, BToken] : TokenPairs)
	{
		if (!AToken->CompareWith(BToken))
		{
			return false;
		}
	}

	// They are equal then :)
	return true;
}

bool UFaerieItem::FindToken(const TSubclassOf<UFaerieItemToken> Class, UFaerieItemToken*& FoundToken) const
{
	if (!IsValid(Class))
	{
		return false;
	}

	// @todo This function is breaking const safety ...
	FoundToken = const_cast<UFaerieItemToken*>(GetToken(Class));
	return FoundToken != nullptr;
}

void UFaerieItem::FindTokens(const TSubclassOf<UFaerieItemToken> Class, TArray<UFaerieItemToken*>& FoundTokens) const
{
	if (!IsValid(Class))
	{
		return;
	}

	// Can't use GetMutableTokens here because it'd fail to return anything if *this* is not data mutable as a precaution.
	// @todo This function is breaking const safety anyways...
	FoundTokens = Type::Cast<TArray<UFaerieItemToken*>>(GetTokens(Class));
}

void UFaerieItem::AddToken(UFaerieItemToken* Token)
{
	if (!ensure(IsValid(Token)))
	{
		return;
	}

	if (!ensure(CanMutate()))
	{
		return;
	}

	if (!ensure(WriteLock == 0))
	{
		UE_LOG(LogFaerieItem, Error, TEXT("Cannot AddToken while iterating in ForEachToken. Please correct code."))
		return;
	}

	// If this check fails, then whatever code tried to add the token didn't create it with us as the outer, or needs to
	// either duplicate or rename the token with us as the outer.
	check(Token->GetOuter() == this);

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastModified, this);
	LastModified = FDateTime::UtcNow();

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Tokens, this);
	Tokens.Add(Token);

	(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Faerie::Tags::TokenAdd);
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

		(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Faerie::Tags::TokenRemove);

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

	if (const int32 Removed = Tokens.RemoveAll(
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
			(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Faerie::Tags::TokenRemove);
		}

		return Removed;
	}

	return 0;
}

/*
EFaerieItemSourceType UFaerieItem::GetSourceType() const
{
	if (GetTypedOuter())
	{
		// @todo how to determine Dynamic / Asset outer
	}
	if (GetPackage() == GetTransientPackage())
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
	(void)NotifyOwnerOfSelfMutation.ExecuteIfBound(this, Token, Faerie::Tags::TokenGenericPropertyEdit);
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