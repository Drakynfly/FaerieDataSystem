﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieItemUsesToken.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

void UFaerieItemUsesToken::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, UsesRemaining, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MaxUses, SharedParams);
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
}

bool UFaerieItemUsesToken::RemoveUses(const int32 Amount)
{
	if (Amount <= 0) return false;

	if (HasUses(Amount))
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, UsesRemaining, this);
		UsesRemaining = FMath::Max(UsesRemaining - Amount, 0);
		NotifyOuterOfChange();
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
}