// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "ItemContainerExtensionEvents.generated.h"

namespace Faerie
{
	enum EExtensionEventType
	{
		Initialization,
		Deinitialization
	};

	using FExtensionEvent = TMulticastDelegate<void(TNotNull<const UFaerieItemContainerBase*> Container, EExtensionEventType Type)>;
	using FPreAdditionEvent = TMulticastDelegate<void(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieItemStackView Stack)>;
	using FPreRemovalEvent = TMulticastDelegate<void(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key, int32 Removal)>;
	using FPostEventBatch = TMulticastDelegate<void(TNotNull<const UFaerieItemContainerBase*> Container, const Inventory::FEventLogBatch& Events)>;
}


/**
 * A simple extension that broadcasts events for Pre- and Post- handlers so that external systems can listen to containers
 */
UCLASS()
class FAERIEINVENTORY_API UItemContainerExtensionEvents : public UItemContainerExtensionBase
{
	GENERATED_BODY()

protected:
	//~ UItemContainerExtensionBase
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual void PreAddition(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieItemStackView Stack) override;
	virtual void PreRemoval(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key, int32 Removal) override;
	virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) override;
	//~ UItemContainerExtensionBase

public:
	Faerie::FExtensionEvent::RegistrationType& GetExtensionEvent() { return ExtensionEvent; }
	Faerie::FPreAdditionEvent::RegistrationType& GetPreAdditionEvent() { return PreAdditionEvent; }
	Faerie::FPreRemovalEvent::RegistrationType& GetPreRemovalEvent() { return PreRemovalEvent; }
	Faerie::FPostEventBatch::RegistrationType& GetOnPostEventBatch() { return OnPostEventBatch; }

private:
	Faerie::FExtensionEvent ExtensionEvent;
	Faerie::FPreAdditionEvent PreAdditionEvent;
	Faerie::FPreRemovalEvent PreRemovalEvent;
	Faerie::FPostEventBatch OnPostEventBatch;
};
