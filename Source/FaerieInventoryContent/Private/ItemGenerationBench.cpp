// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemGenerationBench.h"
#include "FaerieItemMutator.h"
#include "FaerieItemStack.h"
#include "Squirrel.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemGenerationBench)

UItemGenerationBench::UItemGenerationBench()
{
	Squirrel = CreateDefaultSubobject<USquirrel>(FName{TEXTVIEW("Squirrel")});
}

#if WITH_EDITOR

void UItemGenerationBench::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UItemGenerationBench, Drivers))
	{
		for (TObjectPtr<UFaerieItemGenerationConfig>& Driver : Drivers)
		{
			if (!IsValid(Driver))
			{
				Driver = NewObject<UFaerieItemGenerationConfig>(this);
			}
		}
	}
}

#endif

void UItemGenerationBench::RunMutatorOnStack(FFaerieItemStack& Stack)
{
	if (Stack.IsValid() && Mutator.IsValid())
	{
		Mutator.Get().Apply(Stack, Squirrel);
	}
}

FFaerieItemStack UItemGenerationBench::RunMutatorOnStack_Passthrough(const FFaerieItemStack& Stack)
{
	FFaerieItemStack Out = Stack;
	if (Stack.IsValid() && Mutator.IsValid())
	{
		Mutator.Get().Apply(Out, Squirrel);
	}
	return Out;
}
