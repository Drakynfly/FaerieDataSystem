// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "BenchVisualBase.generated.h"

class UBenchBehaviorBase;

/**
 * Minimal example class to demonstrate creating a Bench Behavior instance
 */
UCLASS(Abstract, PrioritizeCategories = "Bench Logic")
class FAERIEINVENTORYCONTENT_API ABenchVisualBase : public AActor
{
	GENERATED_BODY()

public:
	ABenchVisualBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Instanced, Category = "Bench Logic")
	TObjectPtr<UBenchBehaviorBase> Behavior;
};