﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentHashStatics.h"
#include "FaerieEquipmentManager.h"
#include "FaerieEquipmentSlot.h"
#include "Squirrel.h"
#include "DelegateCommon.h"
#include "EquipmentHashAsset.h"
#include "FaerieItemStackHashInstruction.h"
#include "Tokens/FaerieInfoToken.h"

// WARNING: Changing this will invalidate all existing hashes generated with MakeEquipmentHash.
#define EQUIPMENT_HASHING_SEED 561333781

namespace Faerie::Hash
{
	FORCEINLINE [[nodiscard]] int32 Combine(const int32 A, const int32 B)
	{
		return Squirrel::HashCombine(A, B);
	}

	int32 HashFProperty(const void* Ptr, const FProperty* Property)
	{
		if (Property->IsA<FBoolProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<bool>(Ptr));
		}
		if (Property->IsA<FByteProperty>() || Property->IsA<FEnumProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<uint8>(Ptr));
		}
		if (Property->IsA<FIntProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<int32>(Ptr));
		}
		if (Property->IsA<FInt64Property>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<int64>(Ptr));
		}
		if (Property->IsA<FFloatProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<float>(Ptr));
		}
		if (Property->IsA<FDoubleProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<double>(Ptr));
		}
		if (Property->IsA<FNameProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<FName>(Ptr));
		}
		if (Property->IsA<FStrProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<FString>(Ptr));
		}
		if (Property->IsA<FTextProperty>())
		{
			return GetTypeHash(Property->ContainerPtrToValuePtr<FText>(Ptr)->BuildSourceString());
		}
		if (const FStructProperty* AsStruct = CastField<FStructProperty>(Property))
		{
			return HashStructByProps(AsStruct->ContainerPtrToValuePtr<void>(Ptr), AsStruct->Struct, true);
		}
		return 0;
	}

	int32 HashFProperty(const UObject* Obj, const FProperty* Property)
	{
		if (const FBoolProperty* AsBool = CastField<FBoolProperty>(Property))
		{
			return GetTypeHash(*AsBool->ContainerPtrToValuePtr<bool>(Obj));
		}
		return 0;
	}

	template <typename T>
	int32 HashContainerProps(const T* Container, const UStruct* Struct, const bool IncludeSuper)
	{
		int32 Hash = 0;

		for (TFieldIterator<FProperty> PropIt(Struct, IncludeSuper ? EFieldIterationFlags::IncludeSuper : EFieldIterationFlags::None); PropIt; ++PropIt)
		{
			if (const FProperty* Property = *PropIt)
			{
				Hash = Combine(HashFProperty(Container, Property), Hash);
			}
		}

		return Hash;
	}

	int32 HashStructByProps(const void* Ptr, const UScriptStruct* Struct, const bool IncludeSuper)
	{
		check(Ptr);
		check(Struct);
		return HashContainerProps(Ptr, Struct, IncludeSuper);
	}

	int32 HashObjectByProps(const UObject* Obj, const bool IncludeSuper)
	{
		check(Obj);
		return HashContainerProps(Obj, Obj->GetClass(), IncludeSuper);
	}

	FFaerieEquipmentHash MakeEquipmentHash(const TConstArrayView<int32> Hashes)
	{
		// return a 0 hash for an empty array.
		if (Hashes.IsEmpty()) return FFaerieEquipmentHash();

		FFaerieEquipmentHash OutHash{EQUIPMENT_HASHING_SEED};

		// Combine all hashes into the final hash
		for (const int32 Hash : Hashes)
		{
			OutHash.Hash = Combine(Hash, OutHash.Hash);
		}

		return OutHash;
	}

	FFaerieEquipmentHash HashItemSet(const TSet<const UFaerieItem*>& Items,
															  const FEquipmentHashFunction& Function)
	{
		TArray<int32> Hashes;

		for (const UFaerieItem* Item : Items)
		{
			Hashes.Add(Function(Item));
		}

		return MakeEquipmentHash(Hashes);
	}

	FFaerieEquipmentHash HashEquipment(const UFaerieEquipmentManager* Manager,
															const TSet<FFaerieSlotTag>& Slots, const FEquipmentHashFunction& Function)
	{
		if (!IsValid(Manager))
		{
			return FFaerieEquipmentHash();
		}

		TArray<int32> Hashes;

		for (const FFaerieSlotTag SlotTag : Slots)
		{
			if (const UFaerieEquipmentSlot* Slot = Manager->FindSlot(SlotTag, true))
			{
				if (Slot->IsFilled())
				{
					Hashes.Add(Function(Slot->GetItemObject()));
				}
			}
		}

		return MakeEquipmentHash(Hashes);
	}

	bool ExecuteHashInstructions(const UFaerieEquipmentManager* Manager, const UFaerieEquipmentHashAsset* Asset)
	{
		if (!IsValid(Manager) || !IsValid(Asset))
		{
			return false;
		}

		int32 FinalHash = 0;

		for (auto&& Config : Asset->Configs)
		{
			for (const FGameplayTag Tag : Config.Slots)
			{
				const FFaerieSlotTag SlotTag = FFaerieSlotTag::ConvertChecked(Tag);

				int32 TagHash = 0;

				if (auto&& Slot = Manager->FindSlot(SlotTag, true))
				{
					if (Slot->IsFilled())
					{
						TagHash = Config.Instruction->Hash(Slot->View());

						if (Config.MatchType == EGameplayContainerMatchType::Any)
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

	int32 HashItemByName(const UFaerieItem* Item)
	{
		if (!IsValid(Item)) return 0;

		if (const UFaerieInfoToken* InfoToken = Item->GetToken<UFaerieInfoToken>())
		{
			return GetTypeHash(InfoToken->GetItemName().BuildSourceString());
		}

		return 0;
	}
}