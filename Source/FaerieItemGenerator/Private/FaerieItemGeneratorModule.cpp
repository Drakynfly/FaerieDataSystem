// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemGeneratorModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FaerieItemGeneratorModule"

namespace Faerie::Generation
{
#if WITH_EDITOR
	TArray<IMutatorStructTypeCustomizationAutoRegister*> Pending;

	TArray<IMutatorStructTypeCustomizationAutoRegister*> IMutatorStructTypeCustomizationAutoRegister::FlushPending()
	{
		auto PendingCopy = Pending;
		Pending.Empty();
		return PendingCopy;
	}

	void IMutatorStructTypeCustomizationAutoRegister::Register(IMutatorStructTypeCustomizationAutoRegister* Registrar)
	{
		if (FModule* Module = FModuleManager::GetModulePtr<FModule>("FaerieItemGenerator"))
		{
			if (Module->Editor_AddMutatorType.IsBound())
			{
				Module->Editor_AddMutatorType.Execute(Registrar);
				return;
			}
		}

		Pending.Add(Registrar);
	}

	void IMutatorStructTypeCustomizationAutoRegister::Unregister(IMutatorStructTypeCustomizationAutoRegister* Registrar)
	{
		if (FModule* Module = FModuleManager::GetModulePtr<FModule>("FaerieItemGenerator"))
		{
			if (Module->Editor_RemoveMutatorType.IsBound())
			{
				Module->Editor_RemoveMutatorType.Execute(Registrar);
				return;
			}
		}

		Pending.Remove(Registrar);
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