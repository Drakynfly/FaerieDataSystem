// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieChildSlotToken.h"
#include "FaerieEquipmentSlot.h"
#include "ItemContainerExtensionBase.h"
#include "Net/UnrealNetwork.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "FaerieEquipmentSlotDescription.h"
#endif

#include "GameFramework/Actor.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieChildSlotToken)

UFaerieChildSlotToken::UFaerieChildSlotToken()
{
	ItemContainer = CreateDefaultSubobject<UFaerieEquipmentSlot>(FName{TEXTVIEW("ItemContainer")});
	Extensions = CreateDefaultSubobject<UItemContainerExtensionGroup>(FName{TEXTVIEW("Extensions")});
	SET_NEW_IDENTIFIER(Extensions, TEXTVIEW("ChildSlotTokenGroup"))
}

void UFaerieChildSlotToken::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Config, SharedParams);
	DOREPLIFETIME_CONDITION(ThisClass, Extensions, COND_InitialOnly);
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
	Actor->AddReplicatedSubObject(ItemContainer);
	ItemContainer->InitializeNetObject(Actor);

	Extensions->ReplicationFixup();
	Actor->AddReplicatedSubObject(Extensions);
	Extensions->InitializeNetObject(Actor);

	ItemContainer->GetExtensionGroup()->SetParentGroup(Extensions);
}

void UFaerieChildSlotToken::DeinitializeNetObject(AActor* Actor)
{
	ItemContainer->GetExtensionGroup()->SetParentGroup(nullptr);

	Actor->RemoveReplicatedSubObject(ItemContainer);
	ItemContainer->DeinitializeNetObject(Actor);

	Actor->RemoveReplicatedSubObject(Extensions);
	Extensions->DeinitializeNetObject(Actor);
}

UFaerieEquipmentSlot* UFaerieChildSlotToken::GetSlotContainer() const
{
	return Cast<UFaerieEquipmentSlot>(ItemContainer);
}

void UFaerieChildSlotToken::OnSlotItemChanged(UFaerieEquipmentSlot* FaerieEquipmentSlot)
{
	NotifyOuterOfChange();
}