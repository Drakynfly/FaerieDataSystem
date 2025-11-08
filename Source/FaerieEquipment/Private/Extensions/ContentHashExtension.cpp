// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/ContentHashExtension.h"
#include "FaerieInventoryHashStatics.h"
#include "FaerieItemContainerBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "TypeCastingUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ContentHashExtension)

void UContentHashExtension::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ServerChecksum, Params)
}

void UContentHashExtension::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	RecalcContainerHash(Container);
}

void UContentHashExtension::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	PerContainerHash.Remove(Container);
	RecalcLocalChecksum();
}

void UContentHashExtension::LoadSaveData(const UFaerieItemContainerBase* Container, const FInstancedStruct&)
{
	RecalcContainerHash(Container);
}

void UContentHashExtension::PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog&)
{
	RecalcContainerHash(Container);
}

void UContentHashExtension::PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog&)
{
	RecalcContainerHash(Container);
}

void UContentHashExtension::PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog&)
{
	RecalcContainerHash(Container);
}

void UContentHashExtension::RecalcContainerHash(const UFaerieItemContainerBase* Container)
{
	FFaerieHash& Hash = PerContainerHash.FindOrAdd(Container);
	Hash = Faerie::Hash::HashContainer(Container, &Faerie::Hash::HashItemByName);
	RecalcLocalChecksum();
}

void UContentHashExtension::RecalcLocalChecksum()
{
	TArray<uint32> Hashes;
	PerContainerHash.GenerateValueArray(*Type::Cast<TArray<FFaerieHash>*>(&Hashes));

	LocalChecksum = Faerie::Hash::CombineHashes(Hashes);

	if (GetTypedOuter<AActor>()->GetNetMode() < NM_Client)
	{
		// If we are not a client, update and push the server's checksum
		ServerChecksum = LocalChecksum;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ServerChecksum, this);
	}
	else
	{
		// If we are a client, check if this change has put us into or out of sync.
		CheckLocalChecksum();
	}
}

void UContentHashExtension::CheckLocalChecksum()
{
	const bool OldChecksumsMatch = ChecksumsMatch;
	ChecksumsMatch = LocalChecksum == ServerChecksum;

	if (OldChecksumsMatch != ChecksumsMatch)
	{
		const EFaerieChecksumClientState BroadcastState = ChecksumsMatch ?
			EFaerieChecksumClientState::Synchronized :
			EFaerieChecksumClientState::Desynchronized;
		OnClientChecksumEventNative.Broadcast(BroadcastState);
		OnClientChecksumEvent.Broadcast(BroadcastState);
	}
}

void UContentHashExtension::OnRep_ServerChecksum()
{
	CheckLocalChecksum();
}