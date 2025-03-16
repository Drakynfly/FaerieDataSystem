// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "GameplayTagContainer.h"
#include "FaerieTagToken.generated.h"


/**
 * A simple token storing Gameplay Tags on an item. Used by some ItemDataFilters to match items
 */
UCLASS(DisplayName = "Token - Tags")
class FAERIEITEMDATA_API UFaerieTagToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	UFaerieTagToken();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/**
	 * As noted in the parent class, this function rarely needs to be implemented. In this case, gameplay tags might
	 * be being used to distinguish otherwise identical items stacks, even for items with the same name, e.g., giving one
	 * item a "quest"-type tag, should make it stack separately from others like it.
	 */
	virtual bool CompareWithImpl(const UFaerieItemToken* Other) const override
	{
		return CastChecked<ThisClass>(Other)->Tags == Tags;
	}

public:
	static UFaerieTagToken* CreateInstance(const FGameplayTagContainer& Tags);

	const FGameplayTagContainer& GetTags() const { return Tags; }

protected:
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "TagToken", meta = (ExposeOnSpawn))
	FGameplayTagContainer Tags;
};