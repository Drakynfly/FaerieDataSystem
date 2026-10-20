// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMassFragment.h"

#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassFragment)

// Returning false will report to UScriptStruct::SerializeItem that it should fallback to default serialization logic.
// That is fine, as we are only using this as a point to inject the searchable name.
#define FAERIE_IMPLEMENT_SERIALIZE(Type)\
bool Type::Serialize(FArchive& Ar, const UStruct* DefaultsStruct, const void* Defaults)\
{\
	if (Ar.IsSaving())\
	{\
		Ar.MarkSearchableName(Type::StaticStruct(), *DefaultsStruct->GetName());\
	}\
	return false;\
}

FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassFragment)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassSparseFragment)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassTag)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassSparseTag)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassSharedFragment)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassConstSharedFragment)

namespace Faerie::ItemData
{
	bool IsFaerieMassType(const TNotNull<const UScriptStruct*> Type)
	{
		return Type->IsChildOf<FFaerieMassFragment>() ||
			Type->IsChildOf<FFaerieMassSparseFragment>() ||
			Type->IsChildOf<FFaerieMassTag>() ||
			Type->IsChildOf<FFaerieMassSparseTag>() ||
			Type->IsChildOf<FFaerieMassSharedFragment>() ||
			Type->IsChildOf<FFaerieMassConstSharedFragment>();
	}

	bool IsFaerieMassTag(const TNotNull<const UScriptStruct*> Type)
	{
		return Type->IsChildOf<FFaerieMassTag>() ||
			Type->IsChildOf<FFaerieMassSparseTag>();
	}

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

	void IAutoRegisterFragmentTraits::StaticRegisterTraits(const TNotNull<const UScriptStruct*> Type, const FMassTypeTraitsInterface& Interface)
	{
		FragmentTraitsMap.Add(Type, Interface);
	}

	void IAutoRegisterFragmentTraits::StaticDeregisterTraits(const TNotNull<const UScriptStruct*> Type)
	{
		FragmentTraitsMap.Remove(Type);
	}

	const FMassTypeTraitsInterface* GetFragmentTraitsInterface(const TNotNull<const UScriptStruct*> Type)
	{
		if (const FMassTypeTraitsInterface* Traits = FragmentTraitsMap.Find(Type))
		{
			return Traits;
		}
		return nullptr;
	}
}