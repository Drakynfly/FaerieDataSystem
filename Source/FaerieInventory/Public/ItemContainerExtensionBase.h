// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryConcepts.h"
#include "FaerieInventoryTag.h"
#include "FaerieItemDataView.h"
#include "LoopUtils.h"
#include "FaerieStorageEnums.h"
#include "NetSupportedObject.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/SharedStruct.h"

#include "ItemContainerExtensionBase.generated.h"

struct FFaerieItemContainerExtensionData;

namespace Faerie::Container
{
	class IEntryView;
	class IAddressView;
}

namespace Faerie::Inventory
{
	class FEventData;
	class FEventLogBatch;
}

namespace Faerie::Extensions
{
	using FAddressView = ItemData::TNonNullViewPtr<Container::IAddressView>;
	using FEntryView = ItemData::TNonNullViewPtr<Container::IEntryView>;
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

protected:
	virtual FInstancedStruct MakeSaveData(TNotNull<const UFaerieItemContainerBase*> Container) const { return {}; }
	virtual void LoadSaveData(TNotNull<const UFaerieItemContainerBase*> Container, const FInstancedStruct& SaveData) {}

	/* Called at begin play or when the extension is created during runtime */
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) {}
	virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) {}

	/* Does this extension allow a stack of items, or multiple stacks, to be added to the container? */
	virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container,
		TConstArrayView<FFaerieItemDataView> Views, FFaerieExtensionAllowsAdditionArgs Args) const { return EEventExtensionResponse::NoExplicitResponse; }

	/* Allows us to react before an item is added */
	virtual void PreAddition(TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieItemDataView& View) {}

	/* Does this extension allow removal of an address in the container? */
	virtual EEventExtensionResponse AllowsRemoval(TNotNull<const UFaerieItemContainerBase*> Container, Faerie::Extensions::FAddressView DataView, FFaerieInventoryTag Reason) const { return EEventExtensionResponse::NoExplicitResponse; }

	/* Allows us to react before an item is removed */
	virtual void PreRemoval(TNotNull<const UFaerieItemContainerBase*> Container, Faerie::Extensions::FEntryView DataView, int32 Removal) {}

	/* Does this extension allow this entry to be edited? */
	virtual EEventExtensionResponse AllowsEdit(TNotNull<const UFaerieItemContainerBase*> Container, Faerie::Extensions::FAddressView DataView, FFaerieInventoryTag EditTag) const { return EEventExtensionResponse::NoExplicitResponse; }

	// @todo PreEntryChanged

	/* Called after an Addition, Removal, or Change to any address, and carries a full report of each event */
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
		using GroupType = std::conditional_t<Const, const UItemContainerExtensionGroup, UItemContainerExtensionGroup>;
		using ElementType = std::conditional_t<Const, const UItemContainerExtensionBase, UItemContainerExtensionBase>;

	public:
		TExtensionIterator(const TNotNull<GroupType*> Group)
		  : Group(Group)
		{
			operator++();
		}

		UE_REWRITE ElementType* operator*() const { return Current; }

		void operator++();

		UE_REWRITE explicit operator bool() const
		{
			return !!Current;
		}

		[[nodiscard]] UE_REWRITE bool operator!=(Utils::EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

		[[nodiscard]] UE_REWRITE const TExtensionIterator& begin() const { return *this; }
		[[nodiscard]] UE_REWRITE Utils::EIteratorType end() const { return Utils::End; }

	private:
		TNotNull<GroupType*> Group;
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
		using GroupType = std::conditional_t<Const, const UItemContainerExtensionGroup, UItemContainerExtensionGroup>;
		using ElementType = std::conditional_t<Const, const UItemContainerExtensionBase, UItemContainerExtensionBase>;
		using IteratorType = std::conditional_t<Const, typename TArray<TNotNull<ElementType*>>::TConstIterator, typename TArray<TNotNull<ElementType*>>::TIterator>;

	public:
		TRecursiveExtensionIterator(TNotNull<GroupType*> Group);

		static auto GetAllExtensions(TNotNull<GroupType*> Group) -> TArray<TNotNull<ElementType*>>;

		UE_REWRITE TNotNull<ElementType*> operator*() const { return *Iterator; }

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
		TArray<TNotNull<ElementType*>> Extensions;
		IteratorType Iterator;
	};

	using FExtensionIterator = TExtensionIterator<false>;
	using FConstExtensionIterator = TExtensionIterator<true>;

	using FRecursiveExtensionIterator = TRecursiveExtensionIterator<false>;
	using FRecursiveConstExtensionIterator = TRecursiveExtensionIterator<true>;

	struct FGroupAPI;

	FAERIEINVENTORY_API const UItemContainerExtensionBase* Get(const UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch);
	FAERIEINVENTORY_API UItemContainerExtensionBase* Get(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch);

	template <Container::CItemContainerExtension T>
	const T* Get(const UItemContainerExtensionGroup* Group, const bool RecursiveSearch)
	{
		return CastChecked<T>(Get(Group, T::StaticClass(), RecursiveSearch), ECastCheckedType::NullAllowed);
	}

	template <Container::CItemContainerExtension T>
	T* Get(UItemContainerExtensionGroup* Group, const bool RecursiveSearch)
	{
		return CastChecked<T>(Get(Group, T::StaticClass(), RecursiveSearch), ECastCheckedType::NullAllowed);
	}

	// Add a new extension of the given class, and return the result. If an extension of this class already exists, it
	// will be returned instead.
	UItemContainerExtensionBase* AddExtensionByClass(UItemContainerExtensionGroup* Group, TSubclassOf<UItemContainerExtensionBase> ExtensionClass);

	bool RemoveExtensionByClass(UItemContainerExtensionGroup* Group, TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch = true);
}

/*
 * A collection of extensions that implements the interface of the base class to defer to others.
 */
UCLASS()
class FAERIEINVENTORY_API UItemContainerExtensionGroup final : public UItemContainerExtensionBase
{
	GENERATED_BODY()

	friend Faerie::Extensions::FGroupAPI;

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
	virtual void InitializeNetObject(TNotNull<AActor*> Actor) override;
	virtual void DeinitializeNetObject(TNotNull<AActor*> Actor) override;
	//~ UNetSupportedObject

protected:
	//~ UItemContainerExtensionBase
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, TConstArrayView<FFaerieItemDataView> Views, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual void PreAddition(TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieItemDataView& View) override;
	virtual EEventExtensionResponse AllowsRemoval(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Extensions::FAddressView DataView, FFaerieInventoryTag Reason) const override;
	virtual void PreRemoval(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Extensions::FEntryView DataView, int32 Removal) override;
	virtual EEventExtensionResponse AllowsEdit(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Extensions::FAddressView DataView, FFaerieInventoryTag EditTag) const override;
	// @todo PreEntryChanged
	virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) override;
	//~ UItemContainerExtensionBase

public:
	bool AddExtension(UItemContainerExtensionBase* Extension);
	bool RemoveExtension(UItemContainerExtensionBase* Extension);
	bool HasExtension(TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch) const;
	UItemContainerExtensionBase* GetExtension(TSubclassOf<UItemContainerExtensionBase> ExtensionClass, bool RecursiveSearch) const;

	void SetParentGroup(TNotNull<UItemContainerExtensionGroup*> Parent);
	void ClearParentGroup();

	void SetUnclaimedExtensionData(const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData);
	void TryApplyUnclaimedSaveData(UItemContainerExtensionBase* Extension);

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
	TArray<TWeakObjectPtr<const UFaerieItemContainerBase>> Containers;

	// The group that leads "up" the tree of groups when we are a child in a nested container.
	UPROPERTY(Replicated, Transient)
	TObjectPtr<UItemContainerExtensionGroup> ParentGroup;

	// Default subobjects responsible for adding to or customizing container behavior. We always own these.
	UPROPERTY(EditAnywhere, Replicated, Instanced, NoClear, Category = "ExtensionGroup")
	TArray<TObjectPtr<UItemContainerExtensionBase>> Extensions;

	// Additional extensions added during runtime. We do not always own these.
	UPROPERTY(Replicated, Transient)
	TArray<TObjectPtr<UItemContainerExtensionBase>> DynamicExtensions;

	// Save data for extensions that did not exist on us during unraveling.
	TSharedStruct<FFaerieItemContainerExtensionData> UnclaimedExtensionData;
};

namespace Faerie::Extensions
{
	/**
	 * API of UItemContainerExtensionGroup. Use this to call Extension functions that will recurse over all contained
	 * extensions.
	 */
	struct FGroupAPI
	{
		using GroupParam = const TNotNull<UItemContainerExtensionGroup*>;
		using ContainerParam = const TNotNull<const UFaerieItemContainerBase*>;

		UE_REWRITE static void InitializeExtension(GroupParam Group, ContainerParam Container)
		{
			Group->InitializeExtension(Container);
		}

		UE_REWRITE static void DeinitializeExtension(GroupParam Group, ContainerParam Container)
		{
			Group->DeinitializeExtension(Container);
		}

		[[nodiscard]] UE_REWRITE static EEventExtensionResponse AllowsAddition(GroupParam Group, ContainerParam Container,
			const TConstArrayView<FFaerieItemDataView> Views, const FFaerieExtensionAllowsAdditionArgs Args)
		{
			return Group->AllowsAddition(Container, Views, Args);
		}

		UE_REWRITE static void PreAddition(GroupParam Group, ContainerParam Container, const FFaerieItemDataView& View)
		{
			Group->PreAddition(Container, View);
		}

		[[nodiscard]] UE_REWRITE static EEventExtensionResponse AllowsRemoval(GroupParam Group, ContainerParam Container,
			const ItemData::TNonNullViewPtr<Container::IAddressView> DataView, const FFaerieInventoryTag Reason)
		{
			return Group->AllowsRemoval(Container, DataView, Reason);
		}

		UE_REWRITE static void PreRemoval(GroupParam Group, ContainerParam Container, const ItemData::TNonNullViewPtr<Container::IEntryView> DataView, const int32 Removal)
		{
			Group->PreRemoval(Container, DataView, Removal);
		}

		[[nodiscard]] UE_REWRITE static EEventExtensionResponse AllowsEdit(GroupParam Group, ContainerParam Container,
			const ItemData::TNonNullViewPtr<Container::IAddressView> DataView, const FFaerieInventoryTag EditTag)
		{
			return Group->AllowsEdit(Container, DataView, EditTag);
		}

		static void PostEvent(GroupParam Group, ContainerParam Container, const Inventory::FEventData& Event, FFaerieInventoryTag Reason);

		UE_REWRITE static void PostEventBatch(GroupParam Group, ContainerParam Container, const Inventory::FEventLogBatch& Events)
		{
			Group->PostEventBatch(Container, Events);
		}
	};
}