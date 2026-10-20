// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassSubsystemBase.h"

#include "StructUtils/StructView.h"

#include "FaerieMassReplicationSubsystem.generated.h"

struct FFaerieNetItemID;
class AFaerieMassReplicationActor;
class UFaerieItemContainerBase;

/**
 *
 */
UCLASS()
class UFaerieMassReplicationSubsystem : public UMassSubsystemBase
{
	GENERATED_BODY()

protected:
	//~ UWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~ UWorldSubsystem

public:
	void Server_UpdateFragments(FMassEntityHandle Item, const FFaerieNetItemID NetID, TConstArrayView<FConstStructView> FragmentViews);
	void Server_RemoveFragments(FFaerieNetItemID NetID, TConstArrayView<const UScriptStruct*> ScriptStruct);
	void Server_RemoveEntities(TConstArrayView<FFaerieNetItemID> NetIDs);

	TOptional<FMassEntityHandle> ResolveNetId(FFaerieNetItemID NetID) const;

	void ConfirmReplicationActor(AFaerieMassReplicationActor* Actor);

protected:
	UPROPERTY()
	TObjectPtr<AFaerieMassReplicationActor> ReplicationActor;
};
