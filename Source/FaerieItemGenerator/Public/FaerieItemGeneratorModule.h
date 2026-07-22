// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"
#include "Modules/ModuleInterface.h"

namespace Faerie::Generation
{
#if WITH_EDITOR
    struct FAERIEITEMGENERATOR_API IMutatorStructTypeCustomizationAutoRegister
    {
        static TArray<IMutatorStructTypeCustomizationAutoRegister*> FlushPending();
        static void Register(IMutatorStructTypeCustomizationAutoRegister* Registrar);
        static void Unregister(IMutatorStructTypeCustomizationAutoRegister* Registrar);

        UScriptStruct* (*StaticStructAccessor)() = nullptr;
    };

    template <typename T>
    struct TMutatorStructTypeCustomizationAutoRegister : IMutatorStructTypeCustomizationAutoRegister
    {
        TMutatorStructTypeCustomizationAutoRegister()
        {
            StaticStructAccessor = &T::StaticStruct;
            Register(this);
        }

        ~TMutatorStructTypeCustomizationAutoRegister()
        {
            Unregister(this);
        }
    };
#endif

    class FModule : public IModuleInterface
    {
    public:
        virtual void StartupModule() override;
        virtual void ShutdownModule() override;

#if WITH_EDITOR
        using FRegisterMutatorType = TDelegate<void(IMutatorStructTypeCustomizationAutoRegister*)>;
        FRegisterMutatorType Editor_AddMutatorType;
        FRegisterMutatorType Editor_RemoveMutatorType;
#endif
    };
}
