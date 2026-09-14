// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"
#include "Modules/ModuleInterface.h"

namespace Faerie::Generation
{
#if WITH_EDITOR
    struct FAERIEITEMGENERATOR_API IMutatorStructTypeCustomizationAutoRegister
    {
        static void Register(const TNotNull<const UScriptStruct*>);
        static void Unregister(const TNotNull<const UScriptStruct*>);
    };

    template <typename T>
    struct TMutatorStructTypeCustomizationAutoRegister : FDelayedAutoRegisterHelper
    {
        TMutatorStructTypeCustomizationAutoRegister()
          : FDelayedAutoRegisterHelper(EDelayedRegisterRunPhase::EndOfEngineInit, []()
            {
                IMutatorStructTypeCustomizationAutoRegister::Register(T::StaticStruct());
            }) {}

        ~TMutatorStructTypeCustomizationAutoRegister()
        {
            IMutatorStructTypeCustomizationAutoRegister::Unregister(T::StaticStruct());
        }
    };
#endif

    class FModule : public IModuleInterface
    {
    public:
        virtual void StartupModule() override;
        virtual void ShutdownModule() override;

#if WITH_EDITOR
        using FRegisterMutatorType = TDelegate<void(const TNotNull<const UScriptStruct*>)>;
        FRegisterMutatorType Editor_AddMutatorType;
        FRegisterMutatorType Editor_RemoveMutatorType;
#endif
    };
}
