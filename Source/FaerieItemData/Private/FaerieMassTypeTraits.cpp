// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMassTypeTraits.h"

namespace Faerie::ItemData
{
	namespace
	{
		TMap<TNotNull<const UScriptStruct*>, FMassTypeTraitsInterface> FragmentTraitsMap;

		FAutoConsoleCommandWithOutputDevice FDebugPrintFragmentTraitsMap
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
	}

	void IAutoRegisterMassTraits::StaticRegisterTraits(const TNotNull<const UScriptStruct*> Type, const FMassTypeTraitsInterface& Interface)
	{
		FragmentTraitsMap.Add(Type, Interface);
	}

	void IAutoRegisterMassTraits::StaticDeregisterTraits(const TNotNull<const UScriptStruct*> Type)
	{
		FragmentTraitsMap.Remove(Type);
	}

	const FMassTypeTraitsInterface* GetMassTraitsInterface(const TNotNull<const UScriptStruct*> Type)
	{
		for (const UStruct* SuperStruct : Type->GetSuperStructIterator())
		{
			if (const UScriptStruct* ScriptStruct = Cast<UScriptStruct>(SuperStruct))
			{
				if (const FMassTypeTraitsInterface* Traits = FragmentTraitsMap.Find(ScriptStruct))
				{
					return Traits;
				}
			}
		}

		return nullptr;
	}
}