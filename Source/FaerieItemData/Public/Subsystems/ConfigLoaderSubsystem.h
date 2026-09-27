// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassSubsystemBase.h"
#include "ConfigLoaderSubsystem.generated.h"

struct FMassEntityTemplate;
struct FStreamableHandle;
class UMassEntityConfigAsset;

/**
 *
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieMassConfigLoaderSubsystem : public UMassSubsystemBase
{
	GENERATED_BODY()

protected:
	//~ UWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	//~ UWorldSubsystem

public:
	void ForceItemDataTemplateRegistration();

	const FMassEntityTemplate& GetItemDataTemplate();

protected:
	void OnItemDataMassConfigLoaded();

	UPROPERTY()
	TObjectPtr<UMassEntityConfigAsset> ItemDataMassConfig;

	TSharedPtr<FStreamableHandle> ItemDataMassConfigStreamHandle;
};