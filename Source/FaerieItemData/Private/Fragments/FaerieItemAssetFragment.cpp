// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieItemAssetFragment.h"
#include "FaerieItemAsset.h"
#include "MassEntityBuilder.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemAssetFragment)

FAERIE_REGISTER_TRAITS(FFaerieMassItemObject)

bool FFaerieMassItemObject::InitializeRuntime(UE::Mass::FEntityBuilder& Builder)
{
	for (const FInstancedStruct& DefaultFragment : ItemAsset->GetFragmentDefaults())
	{
		const Faerie::ItemData::FMassTypeTraitsInterface* Traits = Faerie::ItemData::GetMassTraitsInterface(DefaultFragment.GetScriptStruct());
		if (Traits && Traits->HasInitializeRuntime)
		{
			FInstancedStruct FragmentCopy = DefaultFragment;
			if (Traits->InitializeRuntime(FragmentCopy.GetMutableMemory(), Builder))
			{
				Builder.Add(MoveTemp(FragmentCopy));
			}
		}
	}
	return true;
}
