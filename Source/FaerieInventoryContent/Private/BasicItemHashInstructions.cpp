// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemHashInstructions.h"
#include "EntityManagerHelpers.h"
#include "FaerieHashStatics.h"
#include "FaerieItem.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemDataView.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemHashInstructions)

// WARNING: Changing any of these will invalidate all existing hashes generated with them.
// These are all random large primes, that will *hopefully* generate decent hashes with Squirrel

#define HASH_FAILURE 0

#define VALIDATED_TRUE 315883619
#define VALIDATED_FALSE 262158943

#define BOOLEAN_FILTER_TRUE 279557143
#define BOOLEAN_FILTER_FALSE 582595723

uint32 UFISHI_Literial::Hash(const TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView&) const
{
	return Value.Hash;
}

uint32 UFISHI_And::Hash(const TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const
{
	int32 Hash = 0;

	for (auto&& Instruction : Instructions)
	{
		Hash = Squirrel::HashCombine(Hash, Instruction->Hash(WorldContextObj, View));
	}

	return Hash;
}

uint32 UFISHI_Or::Hash(const TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const
{
	for (const TObjectPtr<UFaerieItemStackHashInstruction>& Instruction : Instructions)
	{
		if (!Instruction) continue;

		if (const int32 Hash = Instruction->Hash(WorldContextObj, View);
			Hash != HASH_FAILURE)
		{
			return Hash;
		}
	}

	return HASH_FAILURE;
}

uint32 UFISHI_BooleanFilter::Hash(const TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const
{
	if (!ensure(IsValid(Pattern)))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Exec(WorldContextObj, View))
	{
		return BOOLEAN_FILTER_TRUE;
	}

	return BOOLEAN_FILTER_FALSE;
}

uint32 UFISHI_BooleanSelect::Hash(const TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const
{
	if (!ensure(IsValid(Pattern)))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Exec(WorldContextObj, View))
	{
		if (True)
		{
			return True->Hash(WorldContextObj, View);
		}
		return BOOLEAN_FILTER_TRUE;
	}

	if (False)
	{
		return False->Hash(WorldContextObj, View);
	}
	return BOOLEAN_FILTER_FALSE;
}

uint32 UFISHI_Fragments::Hash(const TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const
{
	if (FragmentTypes.IsEmpty()) return 0;

	uint32 Hash = 0;

	const FFaerieItemInstance Instance = View->GetInstance();

	Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	for (auto&& FragmentType : FragmentTypes)
	{
		if (FragmentType->GetCppStructOps()->HasGetTypeHash())
		{
			auto FragmentView = Faerie::ItemData::GetEntityFragmentOrDefault(EntityManager, Instance, FragmentType);
			if (FragmentView.IsValid())
			{
				Hash = Faerie::Hash::Combine(Hash, FragmentType->GetCppStructOps()->GetStructTypeHash(FragmentView.GetMemory()));
			}
		}
	}

	return Hash;
}