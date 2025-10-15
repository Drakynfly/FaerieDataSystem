// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieItemUsesToken.h"
#include "FaerieItem.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemSource.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUsesToken)

class IFaerieItemOwnerInterface;

void FFaerieItemLastUseLogic_Destroy::OnLastUse(UFaerieItem* Item) const
{
	if (IFaerieItemOwnerInterface* Container = Item->GetImplementingOuter<IFaerieItemOwnerInterface>())
	{
		// Release and cast into the aether.
		(void)Container->Release({Item, 1});
	}
}

void FFaerieItemLastUseLogic_Replace::OnLastUse(UFaerieItem* Item) const
{
	if (!IsValid(BaseItemSource.GetObject())) return;

	if (IFaerieItemOwnerInterface* Container = Item->GetImplementingOuter<IFaerieItemOwnerInterface>())
	{
		FFaerieItemInstancingContext Context;
		if (const TOptional<FFaerieItemStack> NewStack = BaseItemSource->CreateItemStack(&Context);
			NewStack.IsSet())
		{
			// Release and cast into the aether.
			(void)Container->Release({Item, Faerie::ItemData::EntireStack});

			// Possess new item
			(void)Container->Possess(NewStack.GetValue());
		}
		else
		{
			UE_LOG(LogItemGeneration, Error, TEXT("Failed to create item instance for FFaerieItemLastUseLogic_Replace on '%s'"), *Item->GetName())
		}
	}
}

void UFaerieItemUsesToken::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, UsesRemaining, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MaxUses, SharedParams);

	DOREPLIFETIME_CONDITION(ThisClass, LastUseLogic, COND_InitialOnly);
}

bool UFaerieItemUsesToken::HasUses(const int32 TestUses) const
{
	return UsesRemaining >= TestUses;
}

void UFaerieItemUsesToken::AddUses(const int32 Amount, const bool ClampRemainingToMax)
{
	if (Amount <= 0) return;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, UsesRemaining, this);
	if (ClampRemainingToMax)
	{
		UsesRemaining = FMath::Min(UsesRemaining + Amount, MaxUses);
	}
	else
	{
		UsesRemaining += Amount;
	}

	NotifyOuterOfChange();

	OnUsesChanged.Broadcast();
}

bool UFaerieItemUsesToken::RemoveUses(const int32 Amount)
{
	if (Amount <= 0) return false;

	if (HasUses(Amount))
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, UsesRemaining, this);
		UsesRemaining = FMath::Max(UsesRemaining - Amount, 0);
		NotifyOuterOfChange();
		OnUsesChanged.Broadcast();

		if (UsesRemaining == 0 && LastUseLogic.IsValid())
		{
			LastUseLogic.Get().OnLastUse(GetOuterItem()->MutateCast());
		}

		return true;
	}
	return false;
}

void UFaerieItemUsesToken::ResetUses()
{
	if (UsesRemaining == MaxUses)
	{
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, UsesRemaining, this);
	UsesRemaining = MaxUses;
	NotifyOuterOfChange();
	OnUsesChanged.Broadcast();
}

void UFaerieItemUsesToken::SetMaxUses(const int32 NewMax, const bool ClampRemainingToMax)
{
	if (NewMax == MaxUses)
	{
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MaxUses, this);
	MaxUses = NewMax;
	if (ClampRemainingToMax)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, UsesRemaining, this);
		UsesRemaining = FMath::Min(UsesRemaining, MaxUses);
	}

	NotifyOuterOfChange();

	OnUsesChanged.Broadcast();
}

void UFaerieItemUsesToken::OnRep_UsesRemaining()
{
	OnUsesChanged.Broadcast();
}
