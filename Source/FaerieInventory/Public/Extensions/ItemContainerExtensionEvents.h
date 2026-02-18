// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "ItemContainerExtensionEvents.generated.h"

namespace Faerie::Extensions
{
	enum EInitializationEventType
	{
		Initialization,
		Deinitialization
	};

	using FInitializationEvent = TMulticastDelegate<void(TNotNull<const UFaerieItemContainerBase*> Container, EInitializationEventType Type)>;
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
	Faerie::Extensions::FInitializationEvent::RegistrationType& GetInitializationEvent() { return ExtensionEvent; }
	Faerie::Extensions::FPreAdditionEvent::RegistrationType& GetPreAdditionEvent() { return PreAdditionEvent; }
	Faerie::Extensions::FPreRemovalEvent::RegistrationType& GetPreRemovalEvent() { return PreRemovalEvent; }
	Faerie::Extensions::FPostEventBatch::RegistrationType& GetOnPostEventBatch() { return OnPostEventBatch; }

private:
	Faerie::Extensions::FInitializationEvent ExtensionEvent;
	Faerie::Extensions::FPreAdditionEvent PreAdditionEvent;
	Faerie::Extensions::FPreRemovalEvent PreRemovalEvent;
	Faerie::Extensions::FPostEventBatch OnPostEventBatch;
};
