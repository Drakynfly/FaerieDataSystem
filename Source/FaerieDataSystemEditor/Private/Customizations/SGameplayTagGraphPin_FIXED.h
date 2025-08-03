// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "EdGraphSchema_K2.h"
#include "KismetPins/SGraphPinStructInstance.h"
#include "GameplayTagContainer.h"

// This class is a fixed version of SGameplayTagGraphPin
class SGameplayTagGraphPin_FIXED : public SGraphPinStructInstance
{
public:
	SLATE_BEGIN_ARGS(SGameplayTagGraphPin_FIXED) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

protected:
	//~ Begin SGraphPinStructInstance Interface
	virtual void ParseDefaultValueData() override;
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

	FGameplayTag GetGameplayTag() const;
	void OnTagChanged(const FGameplayTag NewTag);

	FGameplayTag GameplayTag;
};

// Register SGameplayTagGraphPin_FIXED
class FGameplayTagsGraphPanelPinFactory_ForFix : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override
	{
		if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			if (UScriptStruct* PinStructType = Cast<UScriptStruct>(InPin->PinType.PinSubCategoryObject.Get()))
			{
				if (PinStructType == FGameplayTag::StaticStruct()) return nullptr;

				if (PinStructType->IsChildOf(FGameplayTag::StaticStruct()))
				{
					return SNew(SGameplayTagGraphPin_FIXED, InPin);
				}
			}
		}

		return nullptr;
	}
};