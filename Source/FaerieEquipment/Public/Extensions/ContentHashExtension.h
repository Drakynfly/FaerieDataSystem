// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "ItemContainerExtensionBase.h"
#include "UObject/ObjectKey.h"
#include "ContentHashExtension.generated.h"

class UFaerieItemContainerBase;

UENUM(BlueprintType)
enum class EFaerieChecksumClientState : uint8
{
	Desynchronized,
	Synchronized
};

using FFaerieClientChecksumEventNative = TMulticastDelegate<void(EFaerieChecksumClientState)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFaerieClientChecksumEvent, EFaerieChecksumClientState, State);

/**
 * An item container extension that hashes the contents of initialized containers, and replicates the result.
 */
UCLASS()
class FAERIEEQUIPMENT_API UContentHashExtension : public UItemContainerExtensionBase
{
	GENERATED_BODY()

public:
	//~ UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ UObject

	//~ UItemContainerExtensionBase
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void DeinitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void LoadSaveData(const UFaerieItemContainerBase* Container, const FInstancedStruct& SaveData) override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	//~ UItemContainerExtensionBase

protected:
	void RecalcContainerHash(const UFaerieItemContainerBase* Container);
	void RecalcLocalChecksum();

	void CheckLocalChecksum();

	UFUNCTION(/* Replication */)
	void OnRep_ServerChecksum();

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ContentHashExtension")
	FFaerieHash GetLocalChecksum() const { return LocalChecksum; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContentHashExtension")
	FFaerieHash GetServerChecksum() const { return ServerChecksum; }

	FFaerieClientChecksumEventNative::RegistrationType& GetOnClientChecksumEvent() { return OnClientChecksumEventNative; }

protected:
	UPROPERTY(BlueprintAssignable, Transient, Category = "Events")
	FFaerieClientChecksumEvent OnClientChecksumEvent;

private:
	FFaerieClientChecksumEventNative OnClientChecksumEventNative;

	// For clients, this is the last received hash for the serverside equipment state.
	// For servers, this is identical to LocalChecksum.
	UPROPERTY(ReplicatedUsing = "OnRep_ServerChecksum")
	FFaerieHash ServerChecksum;

	// This is the hash of the current local equipment.
	// This is compared against ServerChecksum each time equipment is changed to verify the client has received the
	// current equipment state.
	FFaerieHash LocalChecksum;

	// Are our checksums known to currently match.
	bool ChecksumsMatch = true;

	TMap<FObjectKey, FFaerieHash> PerContainerHash;
};