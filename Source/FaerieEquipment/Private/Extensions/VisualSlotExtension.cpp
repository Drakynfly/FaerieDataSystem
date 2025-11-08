// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/VisualSlotExtension.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VisualSlotExtension)

void UVisualSlotExtension::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Socket, Params)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ChildSocket, Params)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ComponentTag, Params)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PreferredTag, Params)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, AllowLeaderPose, Params)
}

void UVisualSlotExtension::SetSocket(const FName InSocket)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, Socket, InSocket, this);
}

void UVisualSlotExtension::SetChildSocket(const FName InSocket)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, ChildSocket, InSocket, this);
}

void UVisualSlotExtension::SetComponentTag(const FName InComponentTag)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, ComponentTag, InComponentTag, this);
}

void UVisualSlotExtension::SetPreferredTag(const FGameplayTag InPreferredTag)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, PreferredTag, InPreferredTag, this);
}

void UVisualSlotExtension::SetAllowLeaderPose(const bool InAllowLeaderPose)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, AllowLeaderPose, InAllowLeaderPose, this);
}
