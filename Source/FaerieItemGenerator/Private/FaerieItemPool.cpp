// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemPool.h"
#include "FaerieAssetInfo.h"
#include "ItemInstancingContext_Crafting.h"

#include "Squirrel.h"
#include "Algo/AnyOf.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "FaerieItemGenerationLog.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemPool)

const FFaerieTableDrop* FFaerieWeightedPool::GetDrop(const double RanWeight) const
{
	if (DropList.IsEmpty())
	{
		UE_LOG(LogItemGeneration, Error, TEXT("Exiting generation: Empty Table"));
		return nullptr;
	}

	// Skip performing binary search if there is only one possible result.
	if (DropList.Num() == 1)
	{
		return &DropList[0].Drop;
	}

	const int32 BinarySearchResult = Algo::LowerBoundBy(DropList, RanWeight, &FFaerieWeightedDrop::AdjustedWeight);

	if (!DropList.IsValidIndex(BinarySearchResult))
	{
		UE_LOG(LogItemGeneration, Error, TEXT("Binary search returned out-of-bounds index!"));
		return nullptr;
	}

	return &DropList[BinarySearchResult].Drop;
}

#if WITH_EDITOR
void FFaerieWeightedPool::CalculatePercentages()
{
	/**
	 * Sum all weights into a total weight value, while also adjusting the weight of each drop to include to weight
	 * of all drops before it.
	 */

	int32 WeightSum = 0;
	for (FFaerieWeightedDrop& Entry : DropList)
	{
		WeightSum += Entry.Weight;
		Entry.AdjustedWeight = WeightSum;
	}

	for (FFaerieWeightedDrop& Entry : DropList)
	{
		Entry.AdjustedWeight /= WeightSum;
		Entry.PercentageChanceToDrop = 100.f * (static_cast<float>(Entry.Weight) / static_cast<float>(WeightSum));
	}
}

void FFaerieWeightedPool::SortTable()
{
	Algo::SortBy(DropList, &FFaerieWeightedDrop::AdjustedWeight);
}

namespace Faerie::Editor
{
	bool HasMutableDrops(const TArray<FFaerieWeightedDrop>& Table)
	{
		return Algo::AnyOf(Table,
			[](const FFaerieWeightedDrop& Drop)
			{
				auto&& Interface = Cast<IFaerieItemSource>(Drop.Drop.Asset.Object.LoadSynchronous());
				return Interface && Interface->CanBeMutable();
			});
	}
}

#endif

UFaerieItemPool::UFaerieItemPool()
{
	TableInfo.ObjectName = FText::FromString("<Unnamed Table>");
}

void UFaerieItemPool::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

#if WITH_EDITOR
	DropPool.SortTable();

	HasMutableDrops = Faerie::Editor::HasMutableDrops(DropPool.DropList);
#endif
}

void UFaerieItemPool::PostLoad()
{
	Super::PostLoad();
#if WITH_EDITOR
	DropPool.CalculatePercentages();
#endif
}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemPoolValidation"

EDataValidationResult UFaerieItemPool::IsDataValid(FDataValidationContext& Context) const
{
	TArray<FFaerieItemSourceObject> AssetList;

	for (const FFaerieWeightedDrop& Entry : DropPool.DropList)
	{
		if (!Entry.Drop.IsValid())
		{
			Context.AddWarning(LOCTEXT("DropTableInvalidAsset_Ref", "Invalid Asset Reference"));
		}
		else
		{
			if (AssetList.Contains(Entry.Drop.Asset))
			{
				Context.AddWarning(LOCTEXT("DropTableInvalidAsset_Dup", "Asset already exists in table. Please only have one weight per asset."));
			}
			else
			{
				AssetList.Add(Entry.Drop.Asset);
			}
		}
		if (Entry.Weight <= 0)
		{
			Context.AddWarning(LOCTEXT("DropTableInvalidWeight", "Weight must be larger than 0!"));
		}
	}
	if (Context.GetNumErrors()) return EDataValidationResult::Invalid;
	return Super::IsDataValid(Context);
}

#undef LOCTEXT_NAMESPACE

void UFaerieItemPool::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	DropPool.CalculatePercentages();
}

void UFaerieItemPool::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	DropPool.CalculatePercentages();
}

#endif

bool UFaerieItemPool::CanBeMutable() const
{
#if WITH_EDITOR
	// In the editor, return a value made on the fly. This is so that this works in PIE even after editing an item,
	// but without re-saving this pool yet.
	return Faerie::Editor::HasMutableDrops(DropPool.DropList);
#else
	// At runtime, return the precalculated value.
	return HasMutableDrops;
#endif
}

FFaerieAssetInfo UFaerieItemPool::GetSourceInfo() const
{
	return TableInfo;
}

const UFaerieItem* UFaerieItemPool::CreateItemInstance(const FFaerieItemInstancingContext* Context) const
{
	const FFaerieItemInstancingContext_Crafting* CraftingContent = Context->Cast<FFaerieItemInstancingContext_Crafting>();
	if (!CraftingContent)
	{
		UE_LOG(LogItemGeneration, Error, TEXT("UFaerieItemPool requires a Content of type FItemInstancingContext_Crafting!"));
		return nullptr;
	}

	const FFaerieTableDrop* Drop = [this, CraftingContent]
		{
			if (IsValid(CraftingContent->Squirrel))
			{
				return GetDrop_Seeded(CraftingContent->Squirrel);
			}
			return GetDrop(FMath::FRand());
		}();

	if (Drop && Drop->IsValid())
	{
		return Drop->Resolve(*CraftingContent);
	}

	return nullptr;
}

const FFaerieTableDrop* UFaerieItemPool::GetDrop(const double RanWeight) const
{
	return DropPool.GetDrop(RanWeight);
}

const FFaerieTableDrop* UFaerieItemPool::GetDrop_Seeded(USquirrel* Squirrel) const
{
	return DropPool.GetDrop(Squirrel->NextReal());
}

TConstArrayView<FFaerieWeightedDrop> UFaerieItemPool::ViewDropPool() const
{
	return DropPool.DropList;
}

FFaerieTableDrop UFaerieItemPool::GenerateDrop(const double RanWeight) const
{
	if (auto&& DropPtr = GetDrop(RanWeight))
	{
		return *DropPtr;
	}
	return FFaerieTableDrop();
}

FFaerieTableDrop UFaerieItemPool::GenerateDrop_Seeded(USquirrel* Squirrel) const
{
	if (auto&& DropPtr = GetDrop_Seeded(Squirrel))
	{
		return *DropPtr;
	}
	return FFaerieTableDrop();
}