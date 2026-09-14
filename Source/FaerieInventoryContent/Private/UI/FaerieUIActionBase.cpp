// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "UI/FaerieUIActionBase.h"
#include "FaerieInventoryContentLog.h"
#include "Actions/FaerieInventoryClient.h"
#include "Engine/GameInstance.h"
#include "UI/FaerieStorageWidgetBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieUIActionBase)

void UFaerieUIActionBase::Run_Implementation(UFaerieInventoryClient* Client, const FFaerieItemProxy& Proxy) const {}

EFaerieUIActionState UFaerieUIActionBase::TestCanRun_Implementation(UFaerieInventoryClient* Client, const FFaerieItemProxy& Proxy) const
{
	return EFaerieUIActionState::Enabled;
}

void UFaerieUIActionBase::Finish()
{
	if (!InProgress)
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "Action cannot finish. Is not in progress!")
		return;
	}

	InProgress = false;
}

UFaerieInventoryClient* UFaerieUIActionBase::GetFaerieClient(const UObject* ContextObj)
{
	if (!IsValid(ContextObj))
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "Unable to find Faerie Client. Invalid ContextObj")
		return nullptr;
	}

	const AController* Controller = Cast<AController>(ContextObj);
	if (!IsValid(Controller))
	{
		Controller = ContextObj->GetTypedOuter<AController>();
	}

	if (!IsValid(Controller))
	{
		if (const UUserWidget* OwningWidget = Cast<UUserWidget>(ContextObj))
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

	UE_LOGF(LogFaerieInventoryContent, Error, "Unable to find Faerie Client from: '%ls'", *ContextObj->GetName())
	return nullptr;
}

FText UFaerieUIActionBase::GetDisplayText_Implementation(const FFaerieItemProxy& Proxy) const
{
	return ButtonLabel;
}

TSoftObjectPtr<UTexture2D> UFaerieUIActionBase::GetDisplayIcon_Implementation(const FFaerieItemProxy& Proxy) const
{
	return ButtonIcon;
}

EFaerieUIActionState UFaerieUIActionBase::CanStart(const FFaerieItemProxy& Proxy) const
{
	UFaerieInventoryClient* Client = GetFaerieClient(Cast<UObject>(Proxy.GetItemOwner()));
	if (IsValid(Client))
	{
		return TestCanRun(Client, Proxy);
	}

	// Client doesn't exist. Hide all actions.
	return EFaerieUIActionState::Hidden;
}

bool UFaerieUIActionBase::Start(const FFaerieItemProxy& Proxy)
{
	if (InProgress)
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "Action already in progress!")
		return false;
	}

	UFaerieInventoryClient* Client = GetFaerieClient(Proxy.GetProxyObject());
	if (!IsValid(Client))
	{
		return false;
	}

	InProgress = true;
	Run(Client, Proxy);
	return true;
}
