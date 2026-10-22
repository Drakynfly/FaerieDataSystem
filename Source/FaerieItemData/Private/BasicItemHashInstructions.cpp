// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemHashInstructions.h"
#include "FaerieHashStatics.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"
#include "FaerieItemTemplate.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemHashInstructions)

using namespace Faerie;

// WARNING: Changing any of these will invalidate all existing hashes generated with them.
// These are all random large primes, that will *hopefully* generate decent hashes with Squirrel

#define HASH_FAILURE 0

#define VALIDATED_TRUE 315883619
#define VALIDATED_FALSE 262158943

#define BOOLEAN_FILTER_TRUE 279557143
#define BOOLEAN_FILTER_FALSE 582595723

uint32 FISHI_Literal::Hash(TValid<const FFaerieItemProxy&>) const
{
	return Value.Hash;
}

uint32 FISHI_And::Hash(const TValid<const FFaerieItemProxy&> Proxy) const
{
	int32 Hash = 0;

	for (auto&& Instruction : Instructions)
	{
		Hash = Squirrel::HashCombine(Hash, Instruction->Hash(Proxy));
	}

	return Hash;
}

uint32 FISHI_Or::Hash(const TValid<const FFaerieItemProxy&> Proxy) const
{
	for (auto&& Instruction : Instructions)
	{
		if (!Instruction.IsValid()) continue;

		if (const int32 Hash = Instruction->Hash(Proxy);
			Hash != HASH_FAILURE)
		{
			return Hash;
		}
	}

	return HASH_FAILURE;
}

uint32 FISHI_BooleanFilter::Hash(const TValid<const FFaerieItemProxy&> View) const
{
	if (!ensure(IsValid(Pattern)))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Matches(View))
	{
		return BOOLEAN_FILTER_TRUE;
	}

	return BOOLEAN_FILTER_FALSE;
}

uint32 FISHI_BooleanSelect::Hash(const TValid<const FFaerieItemProxy&> View) const
{
	if (!ensure(IsValid(Pattern)))
	{
		return HASH_FAILURE;
	}

	if (Pattern->Matches(View))
	{
		if (True.IsValid())
		{
			return True->Hash(View);
		}
		return BOOLEAN_FILTER_TRUE;
	}

	if (False.IsValid())
	{
		return False->Hash(View);
	}
	return BOOLEAN_FILTER_FALSE;
}

uint32 FISHI_Fragments::Hash(const TValid<const FFaerieItemProxy&> Proxy) const
{
	if (FragmentTypes.IsEmpty()) return 0;

	uint32 Hash = 0;

	const TOptional<FFaerieItemInstance> Instance = ValidGet(Proxy).GetItemInstance();
	if (!Instance.IsSet()) return 0;

	for (auto&& FragmentType : FragmentTypes)
	{
		if (FragmentType->GetCppStructOps()->HasGetTypeHash())
		{
			const FConstStructView FragmentView = ItemData::GetEntityFragmentOrDefault(ValidGet(Proxy).GetEntityManager().Get(), Instance.GetValue(), FragmentType);
			if (FragmentView.IsValid())
			{
				Hash = Hash::Combine(Hash, FragmentType->GetCppStructOps()->GetStructTypeHash(FragmentView.GetMemory()));
			}
		}
	}

	return Hash;
}