// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/InventoryUIAction.h"
#include "Actions/FaerieInventoryClient.h"
#include "UI/InventoryContentsBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryUIAction)

DEFINE_LOG_CATEGORY(LogInventoryUIAction)

void UInventoryUIAction::Run_Implementation(UFaerieInventoryClient* Client, const FFaerieAddressableHandle Handle) const {}

EInventoryUIActionState UInventoryUIAction::TestCanRun_Implementation(UFaerieInventoryClient* Client, const FFaerieAddressableHandle Handle) const
{
	return EInventoryUIActionState::Enabled;
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

UFaerieInventoryClient* UInventoryUIAction::GetFaerieClient(const UObject* ContextObj)
{
	if (!IsValid(ContextObj))
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Unable to find Faerie Client. Invalid ContextObj"))
		return nullptr;
	}

	const AController* Controller = ContextObj->GetTypedOuter<AController>();

	if (!IsValid(Controller))
	{
		if (const UUserWidget* OwningWidget = ContextObj->GetTypedOuter<UUserWidget>())
		{
			Controller = OwningWidget->GetOwningPlayer();
		}
		else if (const UWorld* World = ContextObj->GetWorld())
		{
			Controller = World->GetGameInstance()->GetLocalPlayerByIndex(0)->GetPlayerController(World);
		}
	}

	if (IsValid(Controller))
	{
		if (UFaerieInventoryClient* Client = Controller->GetComponentByClass<UFaerieInventoryClient>();
			IsValid(Client))
		{
			return Client;
		}
	}

	UE_LOG(LogInventoryUIAction, Error, TEXT("Unable to find Faerie Client from: \'%s\'"), *ContextObj->GetName())
	return nullptr;
}

FText UInventoryUIAction::GetDisplayText_Implementation(const FFaerieAddressableHandle Handle) const
{
	return ButtonLabel;
}

TSoftObjectPtr<UTexture2D> UInventoryUIAction::GetDisplayIcon_Implementation(const FFaerieAddressableHandle Handle) const
{
	return ButtonIcon;
}

EInventoryUIActionState UInventoryUIAction::CanStart(const FFaerieAddressableHandle Handle) const
{
	UFaerieInventoryClient* Client = GetFaerieClient(Handle.Container.Get());
	if (IsValid(Client))
	{
		return TestCanRun(Client, Handle);
	}

	// Client doesn't exist. Hide all actions.
	return EInventoryUIActionState::Hidden;
}

bool UInventoryUIAction::Start(const FFaerieAddressableHandle Handle)
{
	if (InProgress)
	{
		UE_LOG(LogInventoryUIAction, Error, TEXT("Action already in progress!"))
		return false;
	}

	UFaerieInventoryClient* Client = GetFaerieClient(Handle.Container.Get());
	if (!IsValid(Client))
	{
		return false;
	}

	InProgress = true;
	Run(Client, Handle);
	return true;
}
