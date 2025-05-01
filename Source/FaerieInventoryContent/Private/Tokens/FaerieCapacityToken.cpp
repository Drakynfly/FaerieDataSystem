// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieCapacityToken.h"
#include "Extensions/InventoryCapacityExtension.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCapacityToken)

void UFaerieCapacityToken::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, Capacity, COND_InitialOnly);
}

UFaerieCapacityToken* UFaerieCapacityToken::CreateInstance(const FItemCapacity& Capacity)
{
	UFaerieCapacityToken* NewToken = NewObject<UFaerieCapacityToken>();
	NewToken->Capacity = Capacity;
	return NewToken;
}

int32 UFaerieCapacityToken::GetWeightOfStack(const int32 Stack) const
{
	return Capacity.Weight * Stack;
}

int32 UFaerieCapacityToken::GetVolumeOfStack(const int32 Stack) const
{
	const int64 Volume = Capacity.GetVolume();
	return static_cast<int32>(Volume + (Volume * (Stack - 1) * Capacity.Efficiency)); // @todo maybe return int64 here?
}

FWeightAndVolume UFaerieCapacityToken::GetWeightAndVolumeOfStack(const int32 Stack) const
{
	return FWeightAndVolume(GetWeightOfStack(Stack), GetVolumeOfStack(Stack));
}

FWeightAndVolume UFaerieCapacityToken::GetWeightAndVolumeOfStackForRemoval(const int32 Current,
																		   const int32 Removal) const
{
	if (Removal <= 0) return FWeightAndVolume();

	FWeightAndVolume Out;

	if (Removal >= Current)
	{
		Out.GramWeight = GetWeightOfStack(Current);
		Out.Volume = GetVolumeOfStack(Current);
	}
	else
	{
		Out.GramWeight = Capacity.Weight * Removal;
		Out.Volume = static_cast<int64>(Capacity.GetVolume() * Removal * Capacity.Efficiency);
	}

	return Out;
}