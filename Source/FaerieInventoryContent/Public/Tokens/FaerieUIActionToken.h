// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieUIActionToken.generated.h"

class UInventoryUIAction2;

/**
 * Allows Items to declare actions that the UI can call on them.
 */
UCLASS(DisplayName = "Token - UI Actions")
class FAERIEINVENTORYCONTENT_API UFaerieUIActionToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	const TArray<TSubclassOf<UInventoryUIAction2>>& GetActions() const { return Actions; }

protected:
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "UI Actions")
	TArray<TSubclassOf<UInventoryUIAction2>> Actions;
};
