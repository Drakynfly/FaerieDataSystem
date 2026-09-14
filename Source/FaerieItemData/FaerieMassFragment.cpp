// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMassFragment.h"

#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassFragment)

namespace Faerie::ItemData
{
	namespace
	{
		TMap<TNotNull<const UScriptStruct*>, FMassFragmentTypeInterface> FragmentTraitsMap;

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

	void IAutoRegisterFragmentTraits::StaticRegisterTraits(const TNotNull<const UScriptStruct*> Type, const FMassFragmentTypeInterface& Interface)
	{
		FragmentTraitsMap.Add(Type, Interface);
	}

	void IAutoRegisterFragmentTraits::StaticDeregisterTraits(const TNotNull<const UScriptStruct*> Type)
	{
		FragmentTraitsMap.Remove(Type);
	}

	const FMassFragmentTypeInterface* GetFragmentTraitsInterface(const TNotNull<const UScriptStruct*> Type)
	{
		if (const FMassFragmentTypeInterface* Traits = FragmentTraitsMap.Find(Type))
		{
			return Traits;
		}
		return nullptr;
	}
}

bool FFaerieMassFragment::Serialize(FArchive& Ar, const UStruct* DefaultsStruct, const void* Defaults)
{
	if (Ar.IsSaving())
	{
		Ar.MarkSearchableName(FFaerieMassFragment::StaticStruct(), *DefaultsStruct->GetName());
	}

	// Returning false, will report to UScriptStruct::SerializeItem that it should fallback to default serialization logic.
	// That is fine, as we are only using this as a point to inject the searchable name.
	return false;
}
