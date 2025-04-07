// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieChildSlotToken.h"
#include "FaerieEquipmentSlot.h"
#include "ItemContainerExtensionBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "FaerieEquipmentSlotDescription.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieChildSlotToken)

UFaerieChildSlotToken::UFaerieChildSlotToken()
{
	ItemContainer = CreateDefaultSubobject<UFaerieEquipmentSlot>(FName{TEXTVIEW("ItemContainer")});
	Extensions = CreateDefaultSubobject<UItemContainerExtensionGroup>(FName{TEXTVIEW("Extensions")});
	Extensions->SetIdentifier();
}

void UFaerieChildSlotToken::PostLoad()
{
	Super::PostLoad();

	if (!IsTemplate())
	{
		if (auto&& Slot = GetSlotContainer())
		{
			Slot->Config = Config;
			Slot->OnItemChangedNative.AddUObject(this, &ThisClass::OnSlotItemChanged);
		}
	}
}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "ValidateFaerieChildSlotToken"

EDataValidationResult UFaerieChildSlotToken::IsDataValid(FDataValidationContext& Context) const
{
	if (!Config.SlotID.IsValid())
	{
		Context.AddError(LOCTEXT("InvalidSlotID", "Must have valid SlotID set!"));
	}

	if (!IsValid(Config.SlotDescription))
	{
		Context.AddError(LOCTEXT("InvalidSlotDescription", "Must have valid SlotDescription set!"));
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(Context);
}

#undef LOCTEXT_NAMESPACE

#endif

void UFaerieChildSlotToken::InitializeNetObject(AActor* Actor)
{
	ItemContainer->AddExtension(Extensions);
}

UFaerieEquipmentSlot* UFaerieChildSlotToken::GetSlotContainer() const
{
	return Cast<UFaerieEquipmentSlot>(ItemContainer);
}

void UFaerieChildSlotToken::OnSlotItemChanged(UFaerieEquipmentSlot* FaerieEquipmentSlot)
{
	NotifyOuterOfChange();
}