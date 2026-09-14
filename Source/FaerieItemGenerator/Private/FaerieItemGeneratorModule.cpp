// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemGeneratorModule.h"

#include "Modules/ModuleManager.h"
#include "UObject/Class.h"

#define LOCTEXT_NAMESPACE "FaerieItemGeneratorModule"

namespace Faerie::Generation
{
#if WITH_EDITOR
	void IMutatorStructTypeCustomizationAutoRegister::Register(const TNotNull<const UScriptStruct*> StructType)
	{
		checkf(StructType->GetCppStructOps()->HasPostSerialize(), TEXT("Mutator struct must implement PostSerialize! FAERIE_MUTATOR_HEADER is likely missing from header declaration."))

		if (FModule* Module = FModuleManager::GetModulePtr<FModule>("FaerieItemGenerator"))
		{
			if (Module->Editor_AddMutatorType.IsBound())
			{
				Module->Editor_AddMutatorType.Execute(StructType);
			}
		}
	}

	void IMutatorStructTypeCustomizationAutoRegister::Unregister(const TNotNull<const UScriptStruct*> StructType)
	{
		if (FModule* Module = FModuleManager::GetModulePtr<FModule>("FaerieItemGenerator"))
		{
			if (Module->Editor_RemoveMutatorType.IsBound())
			{
				Module->Editor_RemoveMutatorType.Execute(StructType);
			}
		}
	}
#endif

	void FModule::StartupModule()
	{
	}

	void FModule::ShutdownModule()
	{
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(Faerie::Generation::FModule, FaerieItemGenerator)