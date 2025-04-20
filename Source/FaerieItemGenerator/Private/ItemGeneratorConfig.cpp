// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemGeneratorConfig.h"

#include "FaerieItemPool.h"
#include "Squirrel.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemGeneratorConfig)

DEFINE_LOG_CATEGORY(LogItemGenConfig)

UItemGenerationConfig::UItemGenerationConfig()
{
	AmountResolver = TInstancedStruct<FGeneratorAmountBase>::Make<FGeneratorAmount_Fixed>();
}

void UItemGenerationConfig::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

#if WITH_EDITOR
	DropPool.SortTable();
#endif
}

void UItemGenerationConfig::PostLoad()
{
	Super::PostLoad();
#if WITH_EDITOR
	DropPool.CalculatePercentages();
#endif
}

#if WITH_EDITOR
void UItemGenerationConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	DropPool.CalculatePercentages();
}

void UItemGenerationConfig::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	DropPool.CalculatePercentages();
}
#endif

FGeneratorAmountBase UItemGenerationConfig::GetAmountResolver() const
{
	return AmountResolver.Get<FGeneratorAmountBase>();
}

void UItemGenerationConfig::Resolve(TArray<FPendingItemGeneration>& Generations) const
{
	if (const FTableDrop* Drop = DropPool.GetDrop(Squirrel->NextReal()))
	{
		FPendingItemGeneration& Result = Generations.AddDefaulted_GetRef();

		Result.Drop = *Drop;
		Result.Count = AmountResolver.Get<FGeneratorAmountBase>().Resolve(Squirrel);
		Result.Squirrel = Squirrel;

		UE_LOG(LogItemGenConfig, Log, TEXT("Chosen Drop: %s"), *Result.Drop.Asset.Object.ToString());
	}
}