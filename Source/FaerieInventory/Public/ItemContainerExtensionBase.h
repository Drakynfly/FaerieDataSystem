// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerExtensionInterface.h"
#include "FaerieInventoryTag.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemStackView.h"
#include "LoopUtils.h"
#include "InventoryDataEnums.h"
#include "NetSupportedObject.h"
#include "StructUtils/InstancedStruct.h"

#include "ItemContainerExtensionBase.generated.h"

namespace Faerie::Inventory
{
	class FEventLog;
}

UENUM()
enum class EEventExtensionResponse : uint8
{
	// The extension does not care/have authority to allow or deny the event.
	NoExplicitResponse,

	// The extension allows the event
	Allowed,

	// The extension forbids the event
	Disallowed
};

USTRUCT(BlueprintType)
struct FFaerieExtensionAllowsAdditionArgs
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ExtensionAllowsAdditionArgs")
	EFaerieStorageAddStackBehavior AddStackBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ExtensionAllowsAdditionArgs")
	EFaerieStorageAddStackTestMultiType TestType = EFaerieStorageAddStackTestMultiType::IndividualTests; // @todo make this GroupTest by default?
};


/**
 * A modular item container extension. Each extension instance may be registered to multiple UFaerieItemContainerBase
 * objects, so all cached data needs to be sensitive to which container it refers to.
 */
UCLASS(Abstract, HideDropdown, Blueprintable, BlueprintType, EditInlineNew, ClassGroup = "Faerie Inventory", CollapseCategories)
class FAERIEINVENTORY_API UItemContainerExtensionBase : public UNetSupportedObject
{
	GENERATED_BODY()

	friend class UItemContainerExtensionGroup;
	friend class UFaerieItemContainerBase;

public:
	//~ UObject
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	//~ UObject

	virtual void InitializeNetObject(AActor* Actor) override;

protected:
	virtual FInstancedStruct MakeSaveData(const UFaerieItemContainerBase* Container) const { return {}; }
	virtual void LoadSaveData(const UFaerieItemContainerBase* Container, const FInstancedStruct& SaveData) {}

	/* Called at begin play or when the extension is created during runtime */
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) {}
	virtual void DeinitializeExtension(const UFaerieItemContainerBase* Container) {}

	/* Does this extension allow a stack of items, or multiple stacks, to be added to the container? */
	virtual EEventExtensionResponse AllowsAddition(const UFaerieItemContainerBase* Container,
		TConstArrayView<FFaerieItemStackView> Views, FFaerieExtensionAllowsAdditionArgs Args) const { return EEventExtensionResponse::NoExplicitResponse; }

	/* Allows us to react before an item is added */
	virtual void PreAddition(const UFaerieItemContainerBase* Container, FFaerieItemStackView Stack) {}
	/* Allows us to use the key from the last addition */
	virtual void PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) {}

	/* Does this extension allow removal from/of an entry in the container? */
	virtual EEventExtensionResponse AllowsRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryTag Reason) const { return EEventExtensionResponse::NoExplicitResponse; }

	/* Allows us to react before an item is removed */
	virtual void PreRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, int32 Removal) {}
	/* Allows us to use the key from the last removal */
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) {}

	/* Does this extension allow this entry to be edited? */
	virtual EEventExtensionResponse AllowsEdit(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryTag EditTag) const { return EEventExtensionResponse::NoExplicitResponse; }

	// @todo PreEntryChanged

	virtual void PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) {}

public:
	void SetIdentifier(const FGuid* GuidToUse = nullptr);

	FGuid GetIdentifier() const { return Identifier; }

#if WITH_EDITOR
	void SetEditorIdentifier(FStringView StringId);
	FStringView GetEditorIdentifier() const { return EditorIdentifier; }
#endif

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Extension")
	FGuid Identifier;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FString EditorIdentifier;
#endif
};

#if WITH_EDITOR
#define SET_NEW_IDENTIFIER(Ext, StringId)\
	Ext->SetIdentifier();\
	Ext->SetEditorIdentifier(StringId);
#else
#define SET_NEW_IDENTIFIER(Ext, StringId)\
	Ext->SetIdentifier();
#endif

/*
 * A collection of extensions that implements the interface of the base class to defer to others.
 */
UCLASS()
class FAERIEINVENTORY_API UItemContainerExtensionGroup final : public UItemContainerExtensionBase, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

public:
	//~ UObject
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	//~ UObject

	//~ UNetSupportedObject
	virtual void InitializeNetObject(AActor* Actor) override;
	virtual void DeinitializeNetObject(AActor* Actor) override;
	//~ UNetSupportedObject

	//~ UItemContainerExtensionBase
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void DeinitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual EEventExtensionResponse AllowsAddition(const UFaerieItemContainerBase* Container, TConstArrayView<FFaerieItemStackView> Views, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual void PreAddition(const UFaerieItemContainerBase* Container, FFaerieItemStackView Stack) override;
	virtual void PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual EEventExtensionResponse AllowsRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryTag Reason) const override;
	virtual void PreRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, int32 Removal) override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual EEventExtensionResponse AllowsEdit(const UFaerieItemContainerBase* Container, FEntryKey Key, FFaerieInventoryTag EditTag) const override;
	// @todo PreEntryChanged
	virtual void PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	//~ UItemContainerExtensionBase

	//~ IFaerieContainerExtensionInterface
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const override;
	virtual bool AddExtension(UItemContainerExtensionBase* Extension) override;
	virtual bool RemoveExtension(UItemContainerExtensionBase* Extension) override;
	virtual bool HasExtension(TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch) const override;
	virtual UItemContainerExtensionBase* GetExtension(TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch) const override;
	//~ IFaerieContainerExtensionInterface

	void SetParentGroup(UItemContainerExtensionGroup* Parent);

	// Explanation: Extensions are usually pre-configured as instanced subobjects inside a component that is saved to
	// disk in a Blueprint.
	// When these are instantiated, they have the RF_WasLoaded flag, which interferes with replication. It must be removed.
	void ReplicationFixup();

	// Explanation: Cleanup the extensions array after a load to remove stale pointers.
	void ValidateGroup();

	void ForEachExtension(Faerie::TLoop<UItemContainerExtensionBase*> Func);
	void ForEachExtension(Faerie::TLoop<const UItemContainerExtensionBase*> Func) const;

	void ForEachExtensionWithBreak(Faerie::TBreakableLoop<UItemContainerExtensionBase*> Func);
	void ForEachExtensionWithBreak(Faerie::TBreakableLoop<const UItemContainerExtensionBase*> Func) const;

#if !UE_BUILD_SHIPPING
	void PrintDebugData() const;
#endif

private:
	// Containers pointing to this group. This is a transient property populated by InitializeExtension.
	TSet<TWeakObjectPtr<const UFaerieItemContainerBase>> Containers;

	// The group that leads "up" the tree of groups when we are a child in a nested container.
	UPROPERTY(Replicated, Transient)
	TObjectPtr<UItemContainerExtensionGroup> ParentGroup;

	// Default subobjects responsible for adding to or customizing container behavior. We always own these.
	UPROPERTY(EditAnywhere, Replicated, Instanced, NoClear, Category = "ExtensionGroup")
	TArray<TObjectPtr<UItemContainerExtensionBase>> Extensions;

	// Additional extensions added during runtime. We do not always own these.
	UPROPERTY(Replicated, Transient)
	TArray<TObjectPtr<UItemContainerExtensionBase>> DynamicExtensions;
};