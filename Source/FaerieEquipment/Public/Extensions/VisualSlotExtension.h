// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "VisualSlotExtension.generated.h"

/**
 * An extension for configuring the behavior of Visualizers spawned for EquipmentSlots
 */
UCLASS()
class FAERIEEQUIPMENT_API UVisualSlotExtension : public UItemContainerExtensionBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FName GetSocket() const { return Socket; }
	FName GetChildSocket() const { return ChildSocket; }
	FName GetComponentTag() const { return ComponentTag; }
	FGameplayTag GetPreferredTag() const { return PreferredTag; }
	bool GetAllowLeaderPose() const { return AllowLeaderPose; }

	UFUNCTION(BlueprintSetter, Category = "Faerie|VisualSlotExtension")
	void SetSocket(FName InSocket);

	UFUNCTION(BlueprintSetter, Category = "Faerie|VisualSlotExtension")
	void SetChildSocket(FName InSocket);

	UFUNCTION(BlueprintSetter, Category = "Faerie|VisualSlotExtension")
	void SetComponentTag(FName InComponentTag);

	UFUNCTION(BlueprintSetter, Category = "Faerie|VisualSlotExtension")
	void SetPreferredTag(FGameplayTag InPreferredTag);

	UFUNCTION(BlueprintSetter, Category = "Faerie|VisualSlotExtension")
	void SetAllowLeaderPose(bool InAllowLeaderPose);

protected:
	// The socket to attach children to on the parent.
	UPROPERTY(EditAnywhere, Replicated, BlueprintSetter = "SetSocket", Category = "VisualSlotExtension")
	FName Socket;

	// The socket to attach children at on the child.
	UPROPERTY(EditAnywhere, Replicated, BlueprintSetter = "SetSocket", Category = "VisualSlotExtension")
	FName ChildSocket;

	UPROPERTY(EditAnywhere, Replicated, BlueprintSetter = "SetComponentTag", Category = "VisualSlotExtension")
	FName ComponentTag;

	// The MeshPurpose preferred for Visuals attached to this slot.
	UPROPERTY(EditAnywhere, Replicated, BlueprintSetter = "SetPreferredTag", Category = "VisualSlotExtension", meta = (Categories = "MeshPurpose"))
	FGameplayTag PreferredTag;

	UPROPERTY(EditAnywhere, Replicated, BlueprintSetter = "SetAllowLeaderPose", Category = "VisualSlotExtension")
	bool AllowLeaderPose = true;
};