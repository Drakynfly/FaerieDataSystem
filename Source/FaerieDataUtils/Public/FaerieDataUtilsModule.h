// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"
#include "Misc/NotNull.h"
#include "Modules/ModuleInterface.h"

class FAERIEDATAUTILS_API FFaerieDataUtilsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

#if WITH_EDITOR
    void AskEditorToOpenObjectEditorWindow(TNotNull<UObject*> Object) const;

private:
    // The Editor Module will set this up.
    friend class FFaerieInventoryEditorModule;
    using FOnAskEditorToOpenObjectEditorWindow = TDelegate<void(TNotNull<UObject*>)>;
    FOnAskEditorToOpenObjectEditorWindow OnAskEditorToOpenObjectEditorWindow;
#endif
};