// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemGenerator/Public/Generation/FaerieItemGenerationConfig.h"

#include "FaerieItemPool.h"
#include "Squirrel.h"
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemGenerationConfig)

UFaerieItemGenerationConfig::UFaerieItemGenerationConfig()
{
	ProcedureResolver = TInstancedStruct<FFaerieGenerationProcedureBase>::Make<FFaerieGenerationProcedure_OfOne>();
	AmountResolver = TInstancedStruct<FFaerieGeneratorAmountBase>::Make<FFaerieGeneratorAmount_Fixed>();
}

void UFaerieItemGenerationConfig::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

#if WITH_EDITOR
	DropPool.SortTable();
#endif
}

void UFaerieItemGenerationConfig::PostLoad()
{
	Super::PostLoad();
#if WITH_EDITOR
	DropPool.CalculatePercentages();
#endif
}

#if WITH_EDITOR
void UFaerieItemGenerationConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	DropPool.CalculatePercentages();
}

void UFaerieItemGenerationConfig::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	DropPool.CalculatePercentages();
}
#endif

TInstancedStruct<FFaerieGeneratorAmountBase> UFaerieItemGenerationConfig::GetAmountResolver() const
{
	return AmountResolver;
}

void UFaerieItemGenerationConfig::Resolve(TArray<Faerie::FPendingItemGeneration>& Generations, USquirrel* Squirrel) const
{
	if (const FFaerieGenerationProcedureBase* Proc = ProcedureResolver.GetPtr())
	{
		const int32 Amount = [&]() -> int32
		{
			if (const FFaerieGeneratorAmountBase* ResolverPtr = AmountResolver.GetPtr())
			{
				return ResolverPtr->Resolve(Squirrel);
			}
			return 1;
		}();

		if (Amount > 0)
		{
			Proc->Resolve(DropPool, Squirrel, Generations, Amount);
		}
	}
}