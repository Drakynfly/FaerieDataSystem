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

int64 UFaerieCapacityToken::GetVolumeOfStack(const int32 Stack) const
{
	const int64 Volume = Capacity.GetVolume();
	return Volume + static_cast<int64>(Volume * (Stack - 1) * Capacity.Efficiency);
}

int64 UFaerieCapacityToken::GetEfficientVolume(const int32 Stack) const
{
	const int64 Volume = Capacity.GetVolume();
	return static_cast<int64>(Volume * Stack * Capacity.Efficiency);
}

FWeightAndVolume UFaerieCapacityToken::GetWeightAndVolumeOfStack(const int32 Stack) const
{
	return FWeightAndVolume(GetWeightOfStack(Stack), GetVolumeOfStack(Stack));
}

FWeightAndVolume UFaerieCapacityToken::GetWeightAndVolumeOfPartialStack(const int32 Stack) const
{
	return FWeightAndVolume(GetWeightOfStack(Stack), GetEfficientVolume(Stack));
}