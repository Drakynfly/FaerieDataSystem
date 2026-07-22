// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "Templates/SubclassOf.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "FaerieUIActionFragment.generated.h"

class UInventoryUIAction;

USTRUCT(BlueprintType)
struct FFaeriePlayerActionFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI Actions")
	TArray<TSubclassOf<UInventoryUIAction>> Actions;
};

template<>
struct TMassFragmentTraits<FFaeriePlayerActionFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};