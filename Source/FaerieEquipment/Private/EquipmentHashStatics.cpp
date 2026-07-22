// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentHashStatics.h"
#include "EquipmentHashAsset.h"
#include "FaerieEquipmentManager.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItemStackHashInstruction.h"

namespace Faerie::Hash
{
	FFaerieHash HashEquipment(const TNotNull<const UFaerieEquipmentManager*> Manager,
							  const TSet<FFaerieSlotTag>& Slots, const FItemHashFunction& Function)
	{
		TArray<uint32> Hashes;
		Hashes.Reserve(Slots.Num());

		for (const FFaerieSlotTag SlotTag : Slots)
		{
			if (const UFaerieEquipmentSlot* Slot = Manager->FindSlot(SlotTag, true))
			{
				Hashes.Add(Function(Slot, Slot->GetItemInstance().GetValue()));
			}
		}

		return CombineHashes(Hashes);
	}

	bool ExecuteHashInstructions(const TNotNull<const UFaerieEquipmentManager*> Manager, const TNotNull<const UFaerieEquipmentHashAsset*> Asset)
	{
		uint32 FinalHash = 0;

		for (auto&& Config : Asset->Configs)
		{
			bool BreakAfterFirstFilled = Config.MatchType == EGameplayContainerMatchType::Any;

			for (const FGameplayTag Tag : Config.Slots)
			{
				const FFaerieSlotTag SlotTag = FFaerieSlotTag::ConvertChecked(Tag);

				uint32 TagHash = 0;

				if (const UFaerieEquipmentSlot* Slot = Manager->FindSlot(SlotTag, true))
				{
					if (Slot->IsFilled())
					{
						FFaerieItemDataView View(Slot);
						TagHash = Config.Instruction->Hash(Slot, View);

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