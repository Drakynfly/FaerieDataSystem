// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "RelevantActorsExtension.generated.h"

// @todo deprecate this and migrate to DependencyFetcher
/**
 *
 */
UCLASS()
class FAERIEEQUIPMENT_API URelevantActorsExtension : public UItemContainerExtensionBase
{
	GENERATED_BODY()

public:
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void DeinitializeExtension(const UFaerieItemContainerBase* Container) override;

	template <
		typename TActor
		UE_REQUIRES(TIsDerivedFrom<TActor, AActor>::Value)
	>
	TActor* FindActor() const
	{
		return Cast<TActor>(FindActor(TActor::StaticClass()));
	}

	UFUNCTION(BlueprintCallable, Category = "Faerie|RelevantActors", meta = (DeterminesOutputType = "Class"))
	AActor* FindActor(TSubclassOf<AActor> Class) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|RelevantActors")
	void AddActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Faerie|RelevantActors")
	void RemoveActor(AActor* Actor);

protected:
	TSet<TWeakObjectPtr<AActor>> RelevantActors;

	TMap<TWeakObjectPtr<AActor>, int32> OwningActors;
};