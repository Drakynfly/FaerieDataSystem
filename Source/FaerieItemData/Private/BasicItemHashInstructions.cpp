// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemHashInstructions.h"
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

uint32 FISHI_Literal::Hash(const FMassEntityManager*, Faerie::TValid<const FFaerieItemProxy&>) const
{
	return Value.Hash;
}

uint32 FISHI_And::Hash(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemProxy&> View) const
{
	int32 Hash = 0;

	for (auto&& Instruction : Instructions)
	{
		Hash = Squirrel::HashCombine(Hash, Instruction->Hash(EntityManager, View));
	}

	return Hash;
}

uint32 FISHI_Or::Hash(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemProxy&> View) const
{
	for (auto&& Instruction : Instructions)
	{
		if (!Instruction.IsValid()) continue;

		if (const int32 Hash = Instruction->Hash(EntityManager, View);
			Hash != HASH_FAILURE)
		{
			return Hash;
		}
	}

	return HASH_FAILURE;
}

uint32 FISHI_BooleanFilter::Hash(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemProxy&> View) const
{
	if (!ensure(Pattern.IsValid()))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Exec(EntityManager, View))
	{
		return BOOLEAN_FILTER_TRUE;
	}

	return BOOLEAN_FILTER_FALSE;
}

uint32 FISHI_BooleanSelect::Hash(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemProxy&> View) const
{
	if (!ensure(Pattern.IsValid()))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Exec(EntityManager, View))
	{
		if (True.IsValid())
		{
			return True->Hash(EntityManager, View);
		}
		return BOOLEAN_FILTER_TRUE;
	}

	if (False.IsValid())
	{
		return False->Hash(EntityManager, View);
	}
	return BOOLEAN_FILTER_FALSE;
}

uint32 FISHI_Fragments::Hash(const FMassEntityManager* EntityManager, const Faerie::TValid<const FFaerieItemProxy&> View) const
{
	if (FragmentTypes.IsEmpty()) return 0;

	uint32 Hash = 0;

	const TOptional<FFaerieItemInstance> Instance = ValidGet(View).GetItemInstance();
	if (!Instance.IsSet()) return 0;

	for (auto&& FragmentType : FragmentTypes)
	{
		if (FragmentType->GetCppStructOps()->HasGetTypeHash())
		{
			auto FragmentView = Faerie::ItemData::GetEntityFragmentOrDefault(EntityManager, Instance.GetValue(), FragmentType);
			if (FragmentView.IsValid())
			{
				Hash = Faerie::Hash::Combine(Hash, FragmentType->GetCppStructOps()->GetStructTypeHash(FragmentView.GetMemory()));
			}
		}
	}

	return Hash;
}