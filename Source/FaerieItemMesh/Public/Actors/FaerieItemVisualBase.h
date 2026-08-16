// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "FaerieItemProxy.h"
#include "FaerieItemVisualBase.generated.h"

namespace Faerie::Mesh
{
	using FOnVisualActorDisplayFinished = TMulticastDelegate<void(bool)>;
}

/**
 * The base class for actors that visualize a faerie item.
 */
UCLASS(Abstract)
class FAERIEITEMMESH_API AFaerieItemVisualBase : public AActor, public IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	AFaerieItemVisualBase();

	//~ AActor
	virtual USceneComponent* GetDefaultAttachComponent() const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ AActor

public:
	// As this class is abstract it doesn't implement the Faerie::ItemData::IViewBase / IFaerieItemDataProxy interfaces, it just requires that it's children will.
	//~ Faerie::ItemData::IViewBase
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override PURE_VIRTUAL(AFaerieItemVisualBase::GetCopies, return -1; )
	virtual const IFaerieItemOwnerInterface* GetItemOwner() const override PURE_VIRTUAL(AFaerieItemVisualBase::GetItemOwner, return nullptr; )
	//~ Faerie::ItemData::IViewBase

	//~ IFaerieItemDataProxy
	virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() override;
	//~ IFaerieItemDataProxy

	Faerie::Mesh::FOnVisualActorDisplayFinished::RegistrationType& GetOnDisplayFinished() { return OnDisplayFinished; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|ItemRepresentationActor")
	USceneComponent* GetDefaultMeshComponent() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|ItemRepresentationActor")
	void ClearDataDisplay();

	UFUNCTION(BlueprintNativeEvent, Category = "Faerie|ItemRepresentationActor")
	void DisplayData();

	// Function for children to call when its logic for DisplayData has finished running.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemRepresentationActor")
	void NotifyDisplayDataFinished(bool Success = true);

protected:
	void RegenerateDataDisplay();

	Faerie::Mesh::FOnVisualActorDisplayFinished OnDisplayFinished;
};