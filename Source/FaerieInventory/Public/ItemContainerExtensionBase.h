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
	class FEventData;
	class FEventLogBatch;
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
	virtual FInstancedStruct MakeSaveData(TNotNull<const UFaerieItemContainerBase*> Container) const { return {}; }
	virtual void LoadSaveData(TNotNull<const UFaerieItemContainerBase*> Container, const FInstancedStruct& SaveData) {}

	/* Called at begin play or when the extension is created during runtime */
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) {}
	virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) {}

	/* Does this extension allow a stack of items, or multiple stacks, to be added to the container? */
	virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container,
		TConstArrayView<FFaerieItemStackView> Views, FFaerieExtensionAllowsAdditionArgs Args) const { return EEventExtensionResponse::NoExplicitResponse; }

	/* Allows us to react before an item is added */
	virtual void PreAddition(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieItemStackView Stack) {}

	/* Does this extension allow removal of an address in the container? */
	virtual EEventExtensionResponse AllowsRemoval(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieAddress Address, FFaerieInventoryTag Reason) const { return EEventExtensionResponse::NoExplicitResponse; }

	/* Allows us to react before an item is removed */
	virtual void PreRemoval(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key, int32 Removal) {}

	/* Does this extension allow this entry to be edited? */
	virtual EEventExtensionResponse AllowsEdit(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key, FFaerieInventoryTag EditTag) const { return EEventExtensionResponse::NoExplicitResponse; }

	// @todo PreEntryChanged

	/* Called after a Addition, Removal, or Change to any address, and carries a full report of each event */
	virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) {}

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

namespace Faerie::Extensions
{
	// A flat iterator that looks through each extension directly referenced by a group.
	template <bool Const>
	class TExtensionIterator
	{
		using InterfaceType = std::conditional_t<Const, const IFaerieContainerExtensionInterface, IFaerieContainerExtensionInterface>;
		using GroupType = std::conditional_t<Const, const UItemContainerExtensionGroup, UItemContainerExtensionGroup>;
		using ElementType = std::conditional_t<Const, const UItemContainerExtensionBase, UItemContainerExtensionBase>;

	public:
		TExtensionIterator(InterfaceType* Interface)
		  : Group(Interface->GetExtensionGroup())
		{
			operator++();
		}

		TExtensionIterator(GroupType* Group)
		  : Group(Group)
		{
			operator++();
		}

		UE_REWRITE ElementType* operator*() const { return Current; }

		void operator++();

		UE_REWRITE explicit operator bool() const
		{
			return Group && Current;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TExtensionIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	private:
		GroupType* Group;
		ElementType* Current;
		int32 Index;
		enum
		{
			Init,
			Extensions,
			DynamicExtensions,
			ParentGroup
		} State = Init;
	};

	// A recursive iterator that looks through every extension, unraveling groups. Groups themselves are skipped by iteration.
	template <bool Const>
	class TRecursiveExtensionIterator
	{
		using InterfaceType = std::conditional_t<Const, const IFaerieContainerExtensionInterface, IFaerieContainerExtensionInterface>;
		using GroupType = std::conditional_t<Const, const UItemContainerExtensionGroup, UItemContainerExtensionGroup>;
		using ElementType = std::conditional_t<Const, const UItemContainerExtensionBase, UItemContainerExtensionBase>;
		using IteratorType = std::conditional_t<Const, typename TArray<ElementType*>::TConstIterator, typename TArray<ElementType*>::TIterator>;

	public:
		TRecursiveExtensionIterator(InterfaceType* Interface);
		TRecursiveExtensionIterator(GroupType* Group);

		static auto GetAllExtensions(GroupType* Group) -> TArray<ElementType*>;

		UE_REWRITE ElementType* operator*() const { return *Iterator; }

		UE_REWRITE void operator++()
		{
			++Iterator;
		}

		UE_REWRITE explicit operator bool() const
		{
			return static_cast<bool>(Iterator);
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TRecursiveExtensionIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	private:
		TArray<ElementType*> Extensions;
		IteratorType Iterator;
	};

	using FExtensionIterator = TExtensionIterator<false>;
	using FConstExtensionIterator = TExtensionIterator<true>;

	using FRecursiveExtensionIterator = TRecursiveExtensionIterator<false>;
	using FRecursiveConstExtensionIterator = TRecursiveExtensionIterator<true>;
}

/*
 * A collection of extensions that implements the interface of the base class to defer to others.
 */
UCLASS()
class FAERIEINVENTORY_API UItemContainerExtensionGroup final : public UItemContainerExtensionBase, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

	template <bool Const> friend class Faerie::Extensions::TExtensionIterator;
	template <bool Const> friend class Faerie::Extensions::TRecursiveExtensionIterator;

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
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, TConstArrayView<FFaerieItemStackView> Views, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual void PreAddition(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieItemStackView Stack) override;
	virtual EEventExtensionResponse AllowsRemoval(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieAddress Address, FFaerieInventoryTag Reason) const override;
	virtual void PreRemoval(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key, int32 Removal) override;
	virtual EEventExtensionResponse AllowsEdit(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key, FFaerieInventoryTag EditTag) const override;
	// @todo PreEntryChanged
	void PostEvent(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventData& Event, FFaerieInventoryTag Reason);
	virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) override;
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
