// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieGuidToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieGuidToken)

UFaerieGuidToken::UFaerieGuidToken()
{
	GetMutableFaerieItemTokenSparseClassStruct()->ClassTags.AddTag(Faerie::Tags::PrimaryIdentifierToken);
}

void UFaerieGuidToken::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_InitialOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Guid, Params);
}

void UFaerieGuidToken::PostInitProperties()
{
	Super::PostInitProperties();
	Guid = FGuid::NewGuid();
}

void UFaerieGuidToken::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	Guid = FGuid::NewGuid();
}

UFaerieGuidToken* UFaerieGuidToken::CreateInstance(const FGuid* ExistingGuid)
{
	UFaerieGuidToken* NewToken = NewObject<UFaerieGuidToken>();
	if (ExistingGuid)
	{
		NewToken->Guid = *ExistingGuid;
	}
	else
	{
		NewToken->Guid = FGuid::NewGuid();
	}
	return NewToken;
}
