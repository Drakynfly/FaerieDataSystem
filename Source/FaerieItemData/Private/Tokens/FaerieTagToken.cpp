﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieTagToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieTagToken)

UFaerieTagToken::UFaerieTagToken()
{
	GetMutableFaerieItemTokenSparseClassStruct()->ClassTags.AddTag(Faerie::Tags::PrimaryIdentifierToken);
}

void UFaerieTagToken::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_InitialOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Tags, Params);
}

UFaerieTagToken* UFaerieTagToken::CreateInstance(const FGameplayTagContainer& Tags)
{
	UFaerieTagToken* NewToken = NewObject<UFaerieTagToken>();
	NewToken->Tags = Tags;
	return NewToken;
}
