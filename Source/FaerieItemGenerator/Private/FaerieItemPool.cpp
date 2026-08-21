// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemPool.h"
#include "FaerieItemGenerationLog.h"
#include "ItemInstancingContext_Crafting.h"

#include "Squirrel.h"

#include "Algo/AnyOf.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Engine/AssetManager.h"
#include "AssetRegistry/AssetData.h"
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemPool)

#if WITH_EDITOR

namespace Faerie::Editor
{
	/*
	 * Use the Asset Manager to determine if any contained assets are mutable. This avoids having to load them in the editor.
	 */
	bool HasMutableDrops(const TArray<FFaerieWeightedDrop>& Table)
	{
		UAssetManager& AssetManager = UAssetManager::Get();

		return Algo::AnyOf(Table,
			[&AssetManager](const FFaerieWeightedDrop& Drop)
			{
				const FSoftObjectPath Path = Drop.Drop.Asset.Object.ToSoftObjectPath();
				if (Path.IsNull()) return false;

				if (FAssetData AssetData;
					AssetManager.GetAssetDataForPath(Path, AssetData))
				{
					return AssetData.FindTag(IFaerieItemSource::MutableSourceTag);
				}
				return false;
			});
	}
}

#endif

void UFaerieItemPool::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);

	Context.AddTag(FAssetRegistryTag(MutableSourceTag, HasMutableDrops ? TEXT("True") : TEXT("False"), FAssetRegistryTag::TT_Alphabetical));
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
	TSet<FFaerieItemSourceObject> AssetList;

	for (const FFaerieWeightedDrop& Entry : DropPool.DropList)
	{
		if (Entry.Drop.Asset.Object.IsNull())
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

Faerie::ItemData::FGetInstanceResult UFaerieItemPool::CreateItemStack(const FFaerieItemInstancingContext& Context) const
{
	const FFaerieItemInstancingContext_Crafting* CraftingContext = Context.Cast<FFaerieItemInstancingContext_Crafting>();
	if (!CraftingContext)
	{
		UE_LOGF(LogItemGeneration, Error, "UFaerieItemPool requires a Content of type FItemInstancingContext_Crafting!");
		return NullOpt;
	}

	const FFaerieTableDrop* Drop = [this, CraftingContext]
		{
			if (IsValid(CraftingContext->Squirrel))
			{
				return GetDrop_Seeded(CraftingContext->Squirrel->GetState());
			}
			return GetDrop(FMath::FRand());
		}();

	if (Drop && Drop->IsValid())
	{
		return Drop->Resolve(*CraftingContext);
	}

	return NullOpt;
}

const FFaerieTableDrop* UFaerieItemPool::GetDrop(const double RanWeight) const
{
	return DropPool.GetDrop(RanWeight);
}

const FFaerieTableDrop* UFaerieItemPool::GetDrop_Seeded(FSquirrelState& Squirrel) const
{
	return DropPool.GetDrop(Squirrel::NextReal(Squirrel));
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
	if (!ensure(IsValid(Squirrel)))return FFaerieTableDrop();

	if (auto&& DropPtr = GetDrop_Seeded(Squirrel->GetState()))
	{
		return *DropPtr;
	}
	return FFaerieTableDrop();
}