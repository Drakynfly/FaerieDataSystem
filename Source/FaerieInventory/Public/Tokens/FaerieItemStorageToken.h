// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerExtensionInterface.h"
#include "FaerieItemToken.h"
#include "TypeCastingUtils.h"
#include "FaerieItemStorageToken.generated.h"

class UFaerieItemContainerBase;
class UFaerieItemStorage;

/**
 * Base class for tokens that add child item containers to items
 */
UCLASS(Abstract)
class FAERIEINVENTORY_API UFaerieItemContainerToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsMutable() const override;

	// Get all container objects from ContainerTokens.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainerToken")
	static TSet<UFaerieItemContainerBase*> GetAllContainersInItem(const UFaerieItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainerToken", meta = (DeterminesOutputType = Class))
	static TSet<UFaerieItemContainerBase*> GetContainersInItemOfClass(const UFaerieItem* Item, TSubclassOf<UFaerieItemContainerBase> Class);

	template <
		typename TContainerType
		UE_REQUIRES(TIsDerivedFrom<TContainerType, UFaerieItemContainerBase>::Value)
	>
	static TSet<TContainerType*> GetContainersInItem(const UFaerieItem* Item)
	{
		return Type::Cast<TSet<TContainerType*>>(GetContainersInItemOfClass(Item, TContainerType::StaticClass()));
	}

	UFaerieItemContainerBase* GetItemContainer() { return ItemContainer; }
	const UFaerieItemContainerBase* GetItemContainer() const { return ItemContainer; }

protected:
	UPROPERTY(Replicated)
	TObjectPtr<UFaerieItemContainerBase> ItemContainer;
};

class UItemContainerExtensionGroup;

/**
 * This token adds an Item Storage object used to store items nested in another.
 */
UCLASS(DisplayName = "Token - Add Item Storage")
class FAERIEINVENTORY_API UFaerieItemStorageToken : public UFaerieItemContainerToken, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

public:
	UFaerieItemStorageToken();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UNetSupportedObject
	virtual void InitializeNetObject(AActor* Actor) override;
	virtual void DeinitializeNetObject(AActor* Actor) override;
	//~ UNetSupportedObject

	//~ IFaerieContainerExtensionInterface
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const override final { return Extensions; }
	//~ IFaerieContainerExtensionInterface

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStorage")
	UFaerieItemStorage* GetItemStorage() const;

protected:
	UPROPERTY(EditInstanceOnly, Instanced, Replicated, NoClear, Category = "ItemStorage")
	TObjectPtr<UItemContainerExtensionGroup> Extensions;
};