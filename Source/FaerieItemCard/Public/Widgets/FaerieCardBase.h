// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Blueprint/UserWidget.h"
#include "FaerieCardBase.generated.h"

using FOnCardRefreshed = TMulticastDelegate<void()>;

/**
 *
 */
UCLASS(Abstract)
class FAERIEITEMCARD_API UFaerieCardBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	//~ UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ UUserWidget

public:
	FOnCardRefreshed::RegistrationType& GetOnCardRefreshed() { return OnCardRefreshed; }

	void SetItemData(const FFaerieItemProxy& InItemProxy, bool bRefresh);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemCard")
	FFaerieItemProxy GetItemData() const { return ItemProxy; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemCard")
	virtual void Refresh();

protected:
	void OnItemDataChanged(const FFaerieItemProxy& FaerieItemProxy, FGameplayTag GameplayTag);

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemCard", meta = (DisplayName = "Refresh"))
	void BP_Refresh();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "CardWidget")
	FFaerieItemProxy ItemProxy;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CardWidget")
	bool RefreshOnConstruct = true;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "CardWidget")
	bool RefreshOnDataChange = true;

	FOnCardRefreshed OnCardRefreshed;
	FDelegateHandle OnDataChangedHandle;
};