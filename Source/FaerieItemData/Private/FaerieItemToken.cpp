// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemToken.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemToken)

namespace Faerie::Tags
{
	UE_DEFINE_GAMEPLAY_TAG(PrimaryIdentifierToken, "Fae.Token.PrimaryIdentifier");
}

#if WITH_EDITOR
void UFaerieItemToken::PostCDOCompiled(const FPostCDOCompiledContext& Context)
{
	Super::PostCDOCompiled(Context);

	// If we are a Blueprint Token, validate BP functions.
	if (IsInBlueprint())
	{
		for (TFieldIterator<UFunction> FuncIt(GetClass(),
					EFieldIteratorFlags::ExcludeSuper,
					EFieldIteratorFlags::IncludeDeprecated,
					EFieldIteratorFlags::IncludeInterfaces); FuncIt; ++FuncIt)
		{
			const UFunction* Fn = *FuncIt;

			// @todo it would be nice to build out better tooling, so we can be less restrictive here.
			if (!Fn->HasAllFunctionFlags(FUNC_BlueprintPure | FUNC_Const))
			{
				UE_LOG(LogTemp, Error, TEXT(
					"Blueprint Token '%s' contains non-pure/const function '%s'"
					LINE_TERMINATOR
					"All token editing should be done in c++ by UFaerieItemToken::EditToken, or by calling \"Edit Token\" from Blueprint (BP_EditToken)."),
					   *GetName(), *Fn->GetName());
			}
		}
	}
}
#endif

bool UFaerieItemToken::IsMutable() const
{
	return false;
}

bool UFaerieItemToken::CompareWithImpl(const UFaerieItemToken* Other) const
{
	return true;
}

bool UFaerieItemToken::IsOuterItemMutable() const
{
	auto&& OuterItem = GetOuterItem();
	return IsValid(OuterItem) ? OuterItem->CanMutate() : false;
}

void UFaerieItemToken::ReplicateAllPropertiesInitialOnly(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	for (TFieldIterator<FProperty> It(GetClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FProperty* Prop = *It;
		if (Prop != nullptr)
		{
			OutLifetimeProps.AddUnique(
				FLifetimeProperty(Prop->RepIndex,
					COND_InitialOnly,
					REPNOTIFY_OnChanged,
					true));
		}
	}
}

void UFaerieItemToken::NotifyOuterOfChange()
{
	auto&& Item = GetOuterItem();
	if (!Item)
	{
		return;
	}

	Item->OnTokenEdited(this);
}

UFaerieItem* UFaerieItemToken::GetOuterItem() const
{
	return GetTypedOuter<UFaerieItem>();
}

bool UFaerieItemToken::CompareWith(const UFaerieItemToken* Other) const
{
	// Auto-success if we are the same pointer
	if (this == Other) return true;

	// Auto-failure for not matching classes
	if (!IsValid(Other) || !Other->IsA(GetClass())) return false;

	// Run child-implemented data comparison
	return CompareWithImpl(Other);
}

UFaerieItemToken* UFaerieItemToken::MutateCast() const
{
	if (IsMutable())
	{
		return const_cast<ThisClass*>(this);
	}
	return nullptr;
}

void UFaerieItemToken::EditToken(const TFunctionRef<bool(UFaerieItemToken*)>& EditFunc)
{
	if (EditFunc(this))
	{
		NotifyOuterOfChange();
	}
}

UFaerieItem* UFaerieItemToken::BP_GetFaerieItem() const
{
	return GetOuterItem();
}

void UFaerieItemToken::BP_EditToken(const FBlueprintTokenEdit& Edit)
{
	if (!ensure(Edit.IsBound()))
	{
		return;
	}

	EditToken(
		[Edit](UFaerieItemToken* Token)
		{
			return Edit.Execute(Token);
		});
}

UFaerieItemToken* UFaerieItemToken::CreateFaerieItemToken(const TSubclassOf<UFaerieItemToken> TokenClass)
{
	if (TokenClass)
	{
		return NewObject<UFaerieItemToken>(GetTransientPackage(), TokenClass);
	}
	return nullptr;
}
