// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieSlotTag.h"
#include "ItemContainerExtensionBase.h"
#include "MassProcessor.h"

#include "Engine/DataAsset.h"

#include "EquipmentVisualizationUpdater.generated.h"

USTRUCT(BlueprintType)
struct FFaerieVisualSlotElement
{
	GENERATED_BODY()

	// The socket to attach children to on the parent.
	UPROPERTY(EditAnywhere, Category = "VisualSlotElement")
	FName Socket;

	// The socket to attach children at on the child.
	UPROPERTY(EditAnywhere, Category = "VisualSlotElement")
	FName ChildSocket;

	UPROPERTY(EditAnywhere, Category = "VisualSlotElement")
	FName ComponentTag;

	// The MeshPurpose preferred for Visuals attached to this slot.
	UPROPERTY(EditAnywhere, Category = "VisualSlotElement", meta = (Categories = "MeshPurpose"))
	FGameplayTag PreferredTag;

	UPROPERTY(EditAnywhere, Category = "VisualSlotElement")
	bool AllowLeaderPose = true;
};

UCLASS(BlueprintType)
class UFaerieVisualSlotConfiguration : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UE_REWRITE const TMap<FFaerieSlotTag, FFaerieVisualSlotElement>& GetElements() const { return Elements; }

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "VisualSlotConfiguration")
	TMap<FFaerieSlotTag, FFaerieVisualSlotElement> Elements;
};

USTRUCT()
struct FFaerieContainerExtensionVisualUpdater : public FFaerieItemContainerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ContainerExtensionVisualUpdater")
	TObjectPtr<UFaerieVisualSlotConfiguration> Config;

____FAERIE_CONTAINER_DATA_DECL(FFaerieContainerExtensionVisualUpdater)
};

class UEquipmentVisualizer;
class UFaerieItemStackContainer;

UCLASS()
class UFaerieVisualizationUpdater : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieVisualizationUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	static UEquipmentVisualizer* GetVisualizer(TNotNull<const UFaerieItemStackContainer*> Stack);

private:
	FMassEntityQuery EntityQuery;
};