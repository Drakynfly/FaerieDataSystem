// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassReplicationActor.h"
#include "MassEntityTemplate.h"
#include "ValidParameter.h"

#include "Subsystems/WorldSubsystem.h"
#include "FaerieMassReplicationSubsystem.generated.h"

struct FStreamableHandle;
class UMassEntityConfigAsset;
class AFaerieMassReplicationActor;

/**
 *
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieMassReplicationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void ForceItemDataTemplateRegistration();

	void Server_UpdateFragment(Faerie::TValid<const FFaerieItemInstance&> Item, TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews);
	void Server_RemoveFragment(Faerie::TValid<const FFaerieItemInstance&> Item, TNotNull<const UScriptStruct*> ScriptStruct);
	void Server_RemoveEntity(Faerie::TValid<const FFaerieItemInstance&> Item);

	const FMassEntityTemplate& GetItemDataTemplate();

protected:
	void OnItemDataMassConfigLoaded();

	UPROPERTY()
	TObjectPtr<AFaerieMassReplicationActor> ReplicationActor;

	UPROPERTY()
	TObjectPtr<UMassEntityConfigAsset> ItemDataMassConfig;

	TSharedPtr<FStreamableHandle> ItemDataMassConfigStreamHandle;
};
