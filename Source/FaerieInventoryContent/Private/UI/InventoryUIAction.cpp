// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryUIAction.h"
#include "Actions/FaerieInventoryClient.h"
#include "UI/InventoryContentsBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryUIAction)

DEFINE_LOG_CATEGORY(LogInventoryUIAction)

/*
UInventoryUIAction2* UInventoryUIAction2::GetActionInstance(const TSubclassOf<UInventoryUIAction2> Class)
{
	return Class.GetDefaultObject();
}
*/

void UInventoryUIAction::Run_Implementation(const FFaerieAddressableHandle Handle) const {}

void UInventoryUIAction::Finish()
{
	if (!InProgress)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Action cannot finish. Is not in progress!"))
		return;
	}

	InProgress = false;
}

bool UInventoryUIAction::GetFaerieClient(UFaerieInventoryClient*& Client) const
{
	if (const UUserWidget* OwningWidget = GetTypedOuter<UUserWidget>())
	{
		Client = OwningWidget->GetOwningPlayer()->GetComponentByClass<UFaerieInventoryClient>();
		return IsValid(Client);
	}
	return false;
}

FText UInventoryUIAction::GetDisplayText_Implementation(FFaerieAddressableHandle Handle) const
{
	return ButtonLabel;
}

TSoftObjectPtr<UTexture2D> UInventoryUIAction::GetDisplayIcon_Implementation(FFaerieAddressableHandle Handle) const
{
	return ButtonIcon;
}

EInventoryUIActionState UInventoryUIAction::TestCanRun_Implementation(const FFaerieAddressableHandle Handle) const
{
	return EInventoryUIActionState::Enabled;
}

bool UInventoryUIAction::Start(const FFaerieAddressableHandle Handle)
{
	if (InProgress)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Action already in progress!"))
		return false;
	}

	InProgress = true;
	Run(Handle);
	return true;
}
