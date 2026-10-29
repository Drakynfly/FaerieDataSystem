// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ContainerHash/EquipmentHashAsset.h"

#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "FaerieItemAsset.h"
#include "FaerieItemDataView.h"
#include "Squirrel.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentHashAsset)

void UFaerieEquipmentHashAsset::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

#if WITH_EDITOR
	CheckHash = 0;

	for (const FFaerieEquipmentHashAssetConfig& Config : Configs)
	{
		for (int32 i = 0; i < Config.Slots.Num(); ++i)
		{
			int32 TagHash = 0;

			if (Config.Example.IsValidIndex(i) &&
				IsValid(Config.Example[i]))
			{
				const Faerie::ItemData::FScopeProxy StackProxy(Config.Example[i], 1);
				TagHash = Config.Instruction.Hash(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, nullptr, &StackProxy));

				if (Config.MatchType == EGameplayContainerMatchType::Any)
				{
					CheckHash = Squirrel::HashCombine(CheckHash, TagHash);
					break;
				}
			}

			CheckHash = Squirrel::HashCombine(CheckHash, TagHash);
		}
	}
#endif
}