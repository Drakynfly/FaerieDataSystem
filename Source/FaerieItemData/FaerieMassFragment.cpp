// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMassFragment.h"

#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassFragment)

namespace Faerie::ItemData
{
	TArray<IAutoRegisterFragmentTraits*> StaticRegistered;
	TArray<IAutoRegisterFragmentTraits*> StaticDeregistered;

	TMap<TNotNull<const UScriptStruct*>, FMassFragmentTypeInterface*> FragmentTraitsMap;

	static FAutoConsoleCommandWithOutputDevice FDebugPrintFragmentTraitsMap
	(
		TEXT("fae.PrintFragmentTraitsMap"),
		TEXT("Print the map of registered fragment traits to log."),
		FConsoleCommandWithOutputDeviceDelegate::CreateLambda([](FOutputDevice& Output)
			{
				int32 Index = 0;
				for (auto&& FragmentTraits : FragmentTraitsMap)
				{
					Output.Logf(TEXT("[%i]= %ls"), Index, *FragmentTraits.Key->GetName());
					Index++;
				}
			})
	);

	void IAutoRegisterFragmentTraits::StaticRegisterTraits()
	{
		StaticRegistered.Add(this);
	}

	void IAutoRegisterFragmentTraits::StaticDeregisterTraits()
	{
		StaticDeregistered.Add(this);
	}

	void RegisterTraits(const TNotNull<const UScriptStruct*>& Type, FMassFragmentTypeInterface* Traits)
	{
		FragmentTraitsMap.Add(Type, Traits);
	}

	void UnregisterTraits(const TNotNull<const UScriptStruct*>& Type)
	{
		FragmentTraitsMap.Remove(Type);
	}

	void FlushStaticTraitsArrays()
	{
		for (auto&& Registered : StaticRegistered)
		{
			RegisterTraits(Registered->StaticStructAccessor(), &Registered->Interface);
		}
		StaticRegistered.Empty();

		for (auto&& Unregistered : StaticDeregistered)
		{
			UnregisterTraits(Unregistered->StaticStructAccessor());
		}
		StaticDeregistered.Empty();
	}

	const FMassFragmentTypeInterface* GetFragmentTraitsInterface(const TNotNull<const UScriptStruct*> Type)
	{
		// Resolve any statically (de)registered traits lazily.
		FlushStaticTraitsArrays();

		if (FMassFragmentTypeInterface** Traits = FragmentTraitsMap.Find(Type))
		{
			return *Traits;
		}
		return nullptr;
	}
}
