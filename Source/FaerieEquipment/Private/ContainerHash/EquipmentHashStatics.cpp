// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ContainerHash/EquipmentHashStatics.h"
#include "ContainerHash/EquipmentHashAsset.h"
#include "FaerieEquipmentManager.h"

namespace Faerie::Hash
{
	FFaerieHash HashEquipment(const TNotNull<const UFaerieEquipmentManager*> Manager, const FMassEntityManager* EntityManager,
							  const TSet<FFaerieSlotTag>& Slots, const FItemHashFunction& Function)
	{
		TArray<uint32> Hashes;
		Hashes.Reserve(Slots.Num());

		for (const FFaerieSlotTag SlotTag : Slots)
		{
			if (const UFaerieItemStackContainer* Slot = Manager->FindSlot(SlotTag, true))
			{
				Hashes.Add(Function(EntityManager, FFaerieItemProxy(Slot)));
			}
		}

		return CombineHashes(Hashes);
	}

	bool ExecuteHashInstructions(const TNotNull<const UFaerieEquipmentManager*> Manager, const FMassEntityManager* EntityManager, const TNotNull<const UFaerieEquipmentHashAsset*> Asset)
	{
		uint32 FinalHash = 0;

		for (auto&& Config : Asset->Configs)
		{
			bool BreakAfterFirstFilled = Config.MatchType == EGameplayContainerMatchType::Any;

			for (const FGameplayTag Tag : Config.Slots)
			{
				const FFaerieSlotTag SlotTag = FFaerieSlotTag::ConvertChecked(Tag);

				uint32 TagHash = 0;

				if (const UFaerieItemStackContainer* Slot = Manager->FindSlot(SlotTag, true))
				{
					if (Slot->IsFilled())
					{
						TagHash = Config.Instruction.Hash(EntityManager, FFaerieItemProxy(Slot));

						if (BreakAfterFirstFilled)
						{
							FinalHash = Combine(FinalHash, TagHash);
							break;
						}
					}
				}

				FinalHash = Combine(FinalHash, TagHash);
			}
		}

		return FinalHash == Asset->CheckHash;
	}
}