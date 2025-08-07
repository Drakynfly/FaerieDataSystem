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

	using FExtensionEvent = TMulticastDelegate<void(const UFaerieItemContainerBase* Container, EExtensionEventType Type)>;
	using FPreAdditionEvent = TMulticastDelegate<void(const UFaerieItemContainerBase* Container, FFaerieItemStackView Stack)>;
	using FPostAdditionEvent = TMulticastDelegate<void(const UFaerieItemContainerBase* Container, const Inventory::FEventLog& Event)>;
	using FPreRemovalEvent = TMulticastDelegate<void(const UFaerieItemContainerBase* Container, FEntryKey Key, int32 Removal)>;
	using FPostRemovalEvent = TMulticastDelegate<void(const UFaerieItemContainerBase* Container, const Inventory::FEventLog& Event)>;
	using FPostEntryChangedEvent = TMulticastDelegate<void(const UFaerieItemContainerBase* Container, const Inventory::FEventLog& Event)>;
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
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void DeinitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void PreAddition(const UFaerieItemContainerBase* Container, FFaerieItemStackView Stack) override;
	virtual void PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PreRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, int32 Removal) override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	//~ UItemContainerExtensionBase

public:
	Faerie::FExtensionEvent::RegistrationType& GetExtensionEvent() { return ExtensionEvent; }
	Faerie::FPreAdditionEvent::RegistrationType& GetPreAdditionEvent() { return PreAdditionEvent; }
	Faerie::FPostAdditionEvent::RegistrationType& GetPostAdditionEvent() { return PostAdditionEvent; }
	Faerie::FPreRemovalEvent::RegistrationType& GetPreRemovalEvent() { return PreRemovalEvent; }
	Faerie::FPostRemovalEvent::RegistrationType& GetPostRemovalEvent() { return PostRemovalEvent; }
	Faerie::FPostEntryChangedEvent::RegistrationType& GetPostEntryChangedEvent() { return PostEntryChangedEvent; }

private:
	Faerie::FExtensionEvent ExtensionEvent;
	Faerie::FPreAdditionEvent PreAdditionEvent;
	Faerie::FPostAdditionEvent PostAdditionEvent;
	Faerie::FPreRemovalEvent PreRemovalEvent;
	Faerie::FPostRemovalEvent PostRemovalEvent;
	Faerie::FPostEntryChangedEvent PostEntryChangedEvent;
};
