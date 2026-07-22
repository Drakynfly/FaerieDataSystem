// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentHashAsset.h"

#include "UObject/ObjectSaveContext.h"
#include "Squirrel.h"

#if WITH_EDITOR
#include "FaerieItemAsset.h"
#include "FaerieItemDataView.h"
#include "FaerieItemStackHashInstruction.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentHashAsset)

void UFaerieEquipmentHashAsset::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

#if WITH_EDITOR
	CheckHash = 0;

	for (auto&& Config : Configs)
	{
		if (!IsValid(Config.Instruction))
		{
			continue;
		}

		for (int32 i = 0; i < Config.Slots.Num(); ++i)
		{
			int32 TagHash = 0;

			if (Config.Example.IsValidIndex(i) &&
				IsValid(Config.Example[i]))
			{
				const FFaerieItemInstance Instance = Config.Example[i]->GetTemplateInstance();
				const FFaerieItemDataView View(Instance, 1, nullptr);
				TagHash = Config.Instruction->Hash(this, View);

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