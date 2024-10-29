﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieAssetInfo.h"
#include "FaerieItemToken.h"

#include "FaerieInfoToken.generated.h"

UCLASS(DisplayName = "Token - Info")
class FAERIEITEMDATA_API UFaerieInfoToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// This is usually the first and only token needed to determine equivalency, by a simple check of the item's name.
	// Info tokens are inherently immutable, and they must be since they are used to identify items.
	// This doesn't mean that an item *cannot* be renamed, just that if it is, it's considered a separate item.
	virtual bool CompareWithImpl(const UFaerieItemToken* Other) const override;

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|InfoToken")
	const FFaerieAssetInfo& GetInfoObject() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|InfoToken")
	FText GetItemName() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|InfoToken")
	FText GetShortDescription() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|InfoToken")
	FText GetLongDescription() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|InfoToken")
	TSoftObjectPtr<UTexture2D> GetIcon() const;

protected:
	UPROPERTY(EditAnywhere, Replicated, Category = "StaticInfoToken", NoClear)
	FFaerieAssetInfo Info;
};