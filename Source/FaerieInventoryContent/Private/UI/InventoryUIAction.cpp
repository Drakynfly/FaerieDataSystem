// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryUIAction.h"
#include "Actions/FaerieInventoryClient.h"
#include "UI/InventoryContentsBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryUIAction)

DEFINE_LOG_CATEGORY(LogInventoryUIAction)

UWorld* UInventoryUIAction::GetWorld() const
{
	if (ContentsWidget)
	{
		return ContentsWidget->GetWorld();
	}
	return nullptr;
}

void UInventoryUIAction::Setup_Implementation(UInventoryContentsBase* InContentsWidget)
{
	ContentsWidget = InContentsWidget;
}

void UInventoryUIAction::Run_Implementation()
{
	checkf(0, TEXT("Override this!"));
}

void UInventoryUIAction::Finish()
{
	if (!InProgress)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Action cannot finish. Is not in progress!"))
		return;
	}

	InProgress = false;
}

EInventoryUIActionState UInventoryUIAction::CanRunOnEntry_Implementation(const FInventoryKey InKey) const
{
	return EInventoryUIActionState::Enabled;
}

bool UInventoryUIAction::Start(const FInventoryKey InKey)
{
	if (!ContentsWidget)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("ContentsWidget has not been set. Action will not run!"))
		return false;
	}

	if (InProgress)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Action already in progress!"))
		return false;
	}

	Key = InKey;
	InProgress = true;
	Run();
	return true;
}

void UInventoryUIAction2::Run_Implementation() const {}

void UInventoryUIAction2::Finish()
{
	if (!InProgress)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Action cannot finish. Is not in progress!"))
		return;
	}

	InProgress = false;
}

bool UInventoryUIAction2::GetFaerieClient(UFaerieInventoryClient*& Client) const
{
	if (const UUserWidget* OwningWidget = GetTypedOuter<UUserWidget>())
	{
		Client = OwningWidget->GetOwningPlayer()->GetComponentByClass<UFaerieInventoryClient>();
		return IsValid(Client);
	}
	return false;
}

EInventoryUIActionState UInventoryUIAction2::CanRunOnProxy_Implementation(UFaerieItemContainerBase* InContainer, const FEntryKey InKey) const
{
	return EInventoryUIActionState::Enabled;
}

bool UInventoryUIAction2::Start(UFaerieItemContainerBase* InContainer, const FEntryKey InKey)
{
	if (InProgress)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Action already in progress!"))
		return false;
	}

	Container = InContainer;
	Key = InKey;
	InProgress = true;
	Run();
	return true;
}
