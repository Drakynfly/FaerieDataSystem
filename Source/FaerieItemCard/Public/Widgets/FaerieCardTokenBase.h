// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Blueprint/UserWidget.h"
#include "FaerieCardTokenBase.generated.h"

class UFaerieItem;
class UFaerieCardBase;
class UFaerieItemToken;

/**
 *
 */
UCLASS(Abstract)
class FAERIEITEMCARD_API UFaerieCardTokenBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	virtual void OnCardRefreshed();

public:
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemCardToken")
	const UFaerieItem* GetItem() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemCardToken")
	FFaerieItemProxy GetProxy() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemCardToken")
	UFaerieCardBase* GetOwningCard() const;

protected:
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemCardToken", meta = (DynamicOutputParam = "Token", DeterminesOutputType = "Class"))
	UFaerieItemToken* GetItemToken(TSubclassOf<UFaerieItemToken> Class) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemCardToken", meta = (ExpandBoolAsExecs = "ReturnValue", DynamicOutputParam = "Token", DeterminesOutputType = "Class"))
	bool GetItemTokenChecked(UFaerieItemToken*& Token, TSubclassOf<UFaerieItemToken> Class) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemCardToken", meta = (DisplayName = "Refresh"))
	void BP_Refresh();
};