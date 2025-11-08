// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieInfoToken.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieInfoToken)

UFaerieInfoToken::UFaerieInfoToken()
{
	GetMutableFaerieItemTokenSparseClassStruct()->ClassTags.AddTag(Faerie::Tags::PrimaryIdentifierToken);
}

void UFaerieInfoToken::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_InitialOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Info, Params)
}

bool UFaerieInfoToken::CompareWithImpl(const UFaerieItemToken* Other) const
{
	if (auto&& AsInfo = CastChecked<UFaerieInfoToken>(Other))
	{
		if (!Info.ObjectName.IdenticalTo(AsInfo->Info.ObjectName))
		{
			return false;
		}
	}

	return true;
}

uint32 UFaerieInfoToken::GetTokenHashImpl() const
{
	return TextKeyUtil::HashString(Info.ObjectName.BuildSourceString());
}

UFaerieInfoToken* UFaerieInfoToken::CreateInstance(const FFaerieAssetInfo& AssetInfo)
{
	UFaerieInfoToken* NewToken = NewObject<UFaerieInfoToken>();
	NewToken->Info = AssetInfo;
	return NewToken;
}

const FFaerieAssetInfo& UFaerieInfoToken::GetAssetInfo() const
{
	return Info;
}

FText UFaerieInfoToken::GetItemName() const
{
	return Info.ObjectName;
}

FText UFaerieInfoToken::GetShortDescription() const
{
	return Info.ShortDescription;
}

FText UFaerieInfoToken::GetLongDescription() const
{
	return Info.LongDescription;
}

TSoftObjectPtr<UTexture2D> UFaerieInfoToken::GetIcon() const
{
	return Info.Icon;
}