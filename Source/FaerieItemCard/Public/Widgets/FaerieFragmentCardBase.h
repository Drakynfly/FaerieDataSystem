// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Blueprint/UserWidget.h"
#include "FaerieFragmentCardBase.generated.h"

/**
 *
 */
UCLASS(Abstract)
class FAERIEITEMCARD_API UFaerieFragmentCardBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void OnCardRefreshed();

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|FragmentCard")
	class UFaerieCardBase* GetOwningCard() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|FragmentCard")
	FFaerieItemProxy GetProxy() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|FragmentCard", meta = (DisplayName = "Refresh"))
	void BP_Refresh();
};