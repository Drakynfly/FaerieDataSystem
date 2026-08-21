// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemContainerExtensionBase.h"
#include "FaerieItemContainerBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/UObjectThreadContext.h"
#endif

#include "AssetLoadFlagFixer.h"
#include "FaerieInventoryLog.h"
#include "ItemContainerEvent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerExtensionBase)

#define LOCTEXT_NAMESPACE "ItemContainerExtensionGroup"

using namespace Faerie;

void UItemContainerExtensionBase::PostDuplicate(const EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	if (DuplicateMode == EDuplicateMode::PIE) return;

	// Make a new identifier when duplicated (keep editor identifier for debugging)
	SET_NEW_IDENTIFIER(this, GetEditorIdentifier())
}

void UItemContainerExtensionBase::SetIdentifier(const FGuid* GuidToUse)
{
	if (GuidToUse)
	{
		Identifier = *GuidToUse;
	}
	else
	{
		constexpr int64 ExtensionGuidDeterminismSeed = 1;
		Identifier = FGuid::NewDeterministicGuid(GetFullName(), ExtensionGuidDeterminismSeed);
	}
}

#if WITH_EDITOR
void UItemContainerExtensionBase::SetEditorIdentifier(FStringView StringId)
{
	EditorIdentifier = StringId;
}
#endif

namespace Faerie::Extensions
{
	// Declare the permutations of these templates, so we can store the implementations in this file.
	template TExtensionIterator<false>;
	template TExtensionIterator<true>;
	template TRecursiveExtensionIterator<false>;
	template TRecursiveExtensionIterator<true>;

	template <bool Const>
	void TExtensionIterator<Const>::operator++()
	{
		switch (State)
		{
		case Init:
			{
				if (!Group->Extensions.IsEmpty())
				{
					State = Extensions;
					Index = 0;
					Current = Group->Extensions[Index];
					break;
				}
				if (!Group->DynamicExtensions.IsEmpty())
				{
					State = DynamicExtensions;
					Index = 0;
					Current = Group->DynamicExtensions[Index];
					break;
				}
				if (Group->ParentGroup)
				{
					State = ParentGroup;
					Current = Group->ParentGroup;
					break;
				}
				Current = nullptr;
				break;
			}
		case Extensions:
			{
				if (Index < Group->Extensions.Num()-1)
				{
					Index++;
					Current = Group->Extensions[Index];
					break;
				}
				if (!Group->DynamicExtensions.IsEmpty())
				{
					State = DynamicExtensions;
					Index = 0;
					Current = Group->DynamicExtensions[Index];
					break;
				}
				if (Group->ParentGroup)
				{
					State = ParentGroup;
					Current = Group->ParentGroup;
					break;
				}
				Current = nullptr;
				break;
			}
		case DynamicExtensions:
			{
				if (Index < Group->DynamicExtensions.Num()-1)
				{
					Index++;
					Current = Group->DynamicExtensions[Index];
					break;
				}
				if (Group->ParentGroup)
				{
					State = ParentGroup;
					Current = Group->ParentGroup;
					break;
				}
				Current = nullptr;
				break;
			}
		case ParentGroup:
			{
				Current = nullptr;
				break;
			}
		}
	}

	template <bool Const> TRecursiveExtensionIterator<Const>::TRecursiveExtensionIterator(TNotNull<GroupType*> Group)
	  : Extensions(GetAllExtensions(Group)),
		Iterator([this]()
		{
			if constexpr (Const)
			{
				return Extensions.CreateConstIterator();
			}
			else
			{
				return Extensions.CreateIterator();
			}
		}()) {}

	template <bool Const>
	auto TRecursiveExtensionIterator<Const>::GetAllExtensions(const TNotNull<GroupType*> Group) -> TArray<TNotNull<ElementType*>>
	{
		TSet<TNotNull<ElementType*>> AllExtensions;
		TSet<TNotNull<GroupType*>> Searched;
		TArray<TNotNull<GroupType*>> GroupsToSearch;
		GroupsToSearch.Add(Group);
		while (!GroupsToSearch.IsEmpty())
		{
			const TNotNull<GroupType*> GroupToSearch = GroupsToSearch.Pop();
			Searched.Add(GroupToSearch);
			for (const TNotNull<ElementType*> Element : TExtensionIterator<Const>(GroupToSearch))
			{
				if (GroupType* AsGroup = Cast<UItemContainerExtensionGroup>(Element))
				{
					if (!Searched.Contains(AsGroup))
					{
						GroupsToSearch.Push(AsGroup);
					}
				}
				else
				{
					AllExtensions.Add(Element);
				}
			}
		}

		return AllExtensions.Array();
	}

	const UItemContainerExtensionBase* Get(const UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch)
	{
		return Group->GetExtension(Class, RecursiveSearch);
	}

	UItemContainerExtensionBase* Get(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> Class, const bool RecursiveSearch)
	{
		return Group->GetExtension(Class, RecursiveSearch);
	}

	UItemContainerExtensionBase* AddExtensionByClass(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> ExtensionClass)
	{
		if (!ensure(
			IsValid(ExtensionClass) &&
			ExtensionClass != UItemContainerExtensionBase::StaticClass()))
		{
			return nullptr;
		}

		UItemContainerExtensionBase* NewExtension = NewObject<UItemContainerExtensionBase>(Group, ExtensionClass);
		SET_NEW_IDENTIFIER(NewExtension, TEXTVIEW("NewExt:ContainerExtensionInterface"))
		Group->AddExtension(NewExtension);

		return NewExtension;
	}

	bool RemoveExtensionByClass(UItemContainerExtensionGroup* Group, const TSubclassOf<UItemContainerExtensionBase> ExtensionClass, const bool RecursiveSearch)
	{
		if (!ensure(
				IsValid(ExtensionClass) &&
				ExtensionClass != UItemContainerExtensionBase::StaticClass()))
		{
			return false;
		}

		UItemContainerExtensionBase* Extension = Group->GetExtension(ExtensionClass, RecursiveSearch);
		if (!IsValid(Extension))
		{
			return false;
		}

		return Group->RemoveExtension(Extension);
	}
}

void UItemContainerExtensionGroup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ParentGroup, Params)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Extensions, Params)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DynamicExtensions, Params)
}

void UItemContainerExtensionGroup::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	// Jank hack to remove duplicates
	Extensions = TSet<UItemContainerExtensionBase*>(Extensions).Array();

	// Fixes up a whoopsie I did previously
	Extensions.RemoveAll(
		[](const TObjectPtr<UItemContainerExtensionBase>& Extension){ return !IsValid(Extension); });
#endif

	for (auto&& Extension : Extensions)
	{
		if (UItemContainerExtensionGroup* Group = Cast<UItemContainerExtensionGroup>(Extension))
		{
			Group->SetParentGroup(this);
		}
	}
}

#if WITH_EDITOR
EDataValidationResult UItemContainerExtensionGroup::IsDataValid(FDataValidationContext& Context) const
{
	for (auto&& Extension : Extensions)
	{
		if (!IsValid(Extension))
		{
			Context.AddError(LOCTEXT("InvalidExtension", "An Extension is not assigned correctly!"));
		}
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(Context);
}
#endif

void UItemContainerExtensionGroup::InitializeNetObject(const TNotNull<AActor*> Actor)
{
	Super::InitializeNetObject(Actor);

	for (auto&& Extension : Extensions::FExtensionIterator(this))
	{
		Actor->AddReplicatedSubObject(Extension);
		Extension->InitializeNetObject(Actor);
	}
}

void UItemContainerExtensionGroup::DeinitializeNetObject(const TNotNull<AActor*> Actor)
{
	for (auto&& Extension : Extensions::FExtensionIterator(this))
	{
		Actor->RemoveReplicatedSubObject(Extension);
		Extension->DeinitializeNetObject(Actor);
	}

	Super::DeinitializeNetObject(Actor);
}

void UItemContainerExtensionGroup::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	if (Containers.Contains(Container)) return;

#if WITH_EDITOR
	checkf(FUObjectThreadContext::Get().IsInConstructor == false,
		TEXT("Do not call InitializeExtension from a constructor! Use InitializeNetObject if available."));

	// Explanation: For UFaerieInventoryComponents that are added to Blueprint Classes, sometimes in PIE, the
	// component's GEN_VARIABLE version will somehow slip through and try to be added as a container. They are invalid.
	if (Container->GetFullName().Contains("GEN_VARIABLE"))
	{
		//ensure(0);
		return;
	}
#endif

	Containers.Emplace(Container);

	for (auto&& Extension : Extensions::FExtensionIterator(this))
	{
		Extension->InitializeExtension(Container);
	}
}

void UItemContainerExtensionGroup::DeinitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	if (!Containers.Contains(Container)) return;

	for (auto&& Extension : Extensions::FExtensionIterator(this))
	{
		Extension->DeinitializeExtension(Container);
	}

	Containers.Remove(Container);
}

EEventExtensionResponse UItemContainerExtensionGroup::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
																	 const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
																	 const FFaerieExtensionAllowsAdditionArgs Args) const
{
	EEventExtensionResponse Response = EEventExtensionResponse::NoExplicitResponse;

	// Check each extension, to see if the reason is allowed or denied.
	for (auto&& Extension : Extensions::FConstExtensionIterator(this))
	{
		switch (Extension->AllowsAddition(Container, Proxies, Args))
		{
		case EEventExtensionResponse::Allowed:
			{
				// Flag response as allowed, unless another extension bars with a Disallowed
				Response = EEventExtensionResponse::Allowed;
				continue;
			}
		case EEventExtensionResponse::Disallowed:
			{
				// Return false immediately if any Extension bars the reason.
				return EEventExtensionResponse::Disallowed;
			}
		case EEventExtensionResponse::NoExplicitResponse:
		default: break;
		}
	}

	return Response;
}

void UItemContainerExtensionGroup::PreAddition(const TNotNull<const UFaerieItemContainerBase*> Container, const TValid<FFaerieUnownedItemStack>& ItemStack)
{
	for (auto&& Extension : Extensions::FExtensionIterator(this))
	{
		Extension->PreAddition(Container, ItemStack);
	}
}

EEventExtensionResponse UItemContainerExtensionGroup::AllowsRemoval(const TNotNull<const UFaerieItemContainerBase*> Container,
																	const TNotNull<const Container::IAddressView*> DataView,
																	const FFaerieInventoryTag Reason) const
{
	EEventExtensionResponse Response = EEventExtensionResponse::NoExplicitResponse;

	// Check each extension, to see if the reason is allowed or denied.
	for (auto&& Extension : Extensions::FConstExtensionIterator(this))
	{
		switch (Extension->AllowsRemoval(Container, DataView, Reason))
		{
		case EEventExtensionResponse::Allowed:
			{
				// Flag response as allowed, unless another extension bars with a Disallowed
				Response = EEventExtensionResponse::Allowed;
				continue;
			}
		case EEventExtensionResponse::Disallowed:
			{
				// Return false immediately if any Extension bars the reason.
				return EEventExtensionResponse::Disallowed;
			}
		case EEventExtensionResponse::NoExplicitResponse:
		default: break;
		}
	}

	return Response;
}

void UItemContainerExtensionGroup::PreRemoval(const TNotNull<const UFaerieItemContainerBase*> Container,
											  const TNotNull<const Container::IEntryView*> DataView, const int32 Removal)
{
	for (auto&& Extension : Extensions::FExtensionIterator(this))
	{
		Extension->PreRemoval(Container, DataView, Removal);
	}
}

EEventExtensionResponse UItemContainerExtensionGroup::AllowsEdit(const TNotNull<const UFaerieItemContainerBase*> Container,
																 const TNotNull<const Container::IAddressView*> DataView,
																 const FFaerieInventoryTag EditTag) const
{
	EEventExtensionResponse Response = EEventExtensionResponse::NoExplicitResponse;

	// Check each extension, to see if the reason is allowed or denied.
	for (auto&& Extension : Extensions::FConstExtensionIterator(this))
	{
		switch (Extension->AllowsEdit(Container, DataView, EditTag))
		{
		case EEventExtensionResponse::Allowed:
			{
				// Flag response as allowed, unless another extension bars with a Disallowed
				Response = EEventExtensionResponse::Allowed;
				continue;
			}
		case EEventExtensionResponse::Disallowed:
			{
				// Return false immediately if any Extension bars the reason.
				return EEventExtensionResponse::Disallowed;
			}
		case EEventExtensionResponse::NoExplicitResponse:
		default: break;
		}
	}

	return Response;
}

void UItemContainerExtensionGroup::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container,
	const Inventory::FEventLogBatch& Events)
{
	for (auto&& Extension : Extensions::FExtensionIterator(this))
	{
		Extension->PostEventBatch(Container, Events);
	}
}

#if !UE_BUILD_SHIPPING

void UItemContainerExtensionGroup::PrintDebugData() const
{
	const AActor* OwningActor = GetTypedOuter<AActor>();

	UE_LOGF(LogFaerieInventory, Log, "Printing Containers/Extension in Group (%ls)",
		OwningActor ? *("Role: " + UEnum::GetValueAsString<ENetRole>(OwningActor->GetLocalRole())) : TEXT("No Owner"))
	for (auto&& Container : Containers)
	{
		if (!Container.IsValid())
		{
			UE_LOGF(LogFaerieInventory, Warning,  "	Invalid Containers in PrintDebugData. Investigate!")
			continue;
		}
		UE_LOGF(LogFaerieInventory, Warning,  "	Registered Container: '%ls'", *Container->GetName())
	}

	for (auto&& Extension : Extensions)
	{
		if (!IsValid(Extension))
		{
			UE_LOGF(LogFaerieInventory, Warning,  "	Invalid Extension in PrintDebugData. Investigate!")
			continue;
		}
		UE_LOGF(LogFaerieInventory, Warning,  "	Registered Extension: '%ls'", *Extension->GetName())
	}
	for (auto&& Extension : DynamicExtensions)
	{
		if (!IsValid(Extension))
		{
			UE_LOGF(LogFaerieInventory, Warning,  "	Invalid Extension in PrintDebugData. Investigate!")
			continue;
		}
		UE_LOGF(LogFaerieInventory, Warning,  "	Registered Extension: '%ls'", *Extension->GetName())
	}
}

#endif

bool UItemContainerExtensionGroup::AddExtension(UItemContainerExtensionBase* Extension)
{
	if (!ensure(IsValid(Extension))) return false;

	checkf(Extension->GetIdentifier().IsValid(),
		TEXT("Extension with invalid Identifier. Setup code-path with SetIdentifier called before AddExtension"))

	if (!ensureAlwaysMsgf(!Extensions.Contains(Extension),
		TEXT("Trying to add a dynamic Extension that is already in default. This is bad. Track down why this was attempted!")))
	{
		return false;
	}
	if (!ensureAlwaysMsgf(!DynamicExtensions.Contains(Extension),
		TEXT("Trying to add dynamic Extension twice. This is bad. Track down why this was attempted!")))
	{
		return false;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Extensions, this);
	DynamicExtensions.Add(Extension);
	TryApplyUnclaimedSaveData(Extension);

	for (auto&& Container : Containers)
	{
		if (Container.IsValid())
		{
			Extension->InitializeExtension(Container.Get());
		}
	}
	return true;
}

bool UItemContainerExtensionGroup::RemoveExtension(UItemContainerExtensionBase* Extension)
{
	if (!ensure(IsValid(Extension))) return false;

	if (!ensureAlwaysMsgf(!Extensions.Contains(Extension),
		TEXT("Trying to remove a default Extension. This is not allowed. Track down why this was attempted!")))
	{
		return false;
	}

	// Ignore attempts to remove an extension we don't contain.
	if (!DynamicExtensions.Contains(Extension)) return false;

	for (auto&& Container : Containers)
	{
		if (Container.IsValid())
		{
			Extension->DeinitializeExtension(Container.Get());
		}
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Extensions, this);
	return !!DynamicExtensions.Remove(Extension);
}

bool UItemContainerExtensionGroup::HasExtension(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass, const bool RecursiveSearch) const
{
	if (!ensure(
	IsValid(ExtensionClass) &&
	ExtensionClass != UItemContainerExtensionBase::StaticClass()))
	{
		return false;
	}

	if (!IsValid(ExtensionClass) || ExtensionClass == UItemContainerExtensionBase::StaticClass()) return false;

	for (auto&& Extension : Extensions::FConstExtensionIterator(this))
	{
		// Find extension by direct search
		if (Extension->IsA(ExtensionClass))
		{
			return true;
		}

		if (RecursiveSearch)
		{
			// Find extension via recursive search
			if (auto&& Group = Cast<ThisClass>(Extension))
			{
				if (Group->HasExtension(ExtensionClass, true))
				{
					return true;
				}
			}
		}
	}

	return false;
}

UItemContainerExtensionBase* UItemContainerExtensionGroup::GetExtension(const TSubclassOf<UItemContainerExtensionBase> ExtensionClass, const bool RecursiveSearch) const
{
	if (!IsValid(ExtensionClass) || ExtensionClass == UItemContainerExtensionBase::StaticClass()) return nullptr;

	for (auto&& Extension : Extensions::FConstExtensionIterator(this))
	{
		// Find extension by direct search
		if (Extension->IsA(ExtensionClass))
		{
			return const_cast<UItemContainerExtensionBase*>(Extension);
		}

		if (RecursiveSearch)
		{
			// Find extension via recursive search
			if (auto&& Group = Cast<ThisClass>(Extension))
			{
				if (auto&& Found = Group->GetExtension(ExtensionClass, true))
				{
					return Found;
				}
			}
		}
	}

	return nullptr;
}

void UItemContainerExtensionGroup::SetParentGroup(TNotNull<UItemContainerExtensionGroup*> Parent)
{
	checkf(Parent->GetIdentifier().IsValid(),
		TEXT("Parent with invalid Identifier. Setup code-path with SetIdentifier called before SetParentGroup"))

	if (!ensureAlwaysMsgf(!IsValid(ParentGroup),
		TEXT("Attempted to set parent group while one is already assigned!")))
	{
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ParentGroup, this);
	ParentGroup = Parent;
	for (auto&& Container : Containers)
	{
		if (Container.IsValid())
		{
			ParentGroup->InitializeExtension(Container.Get());
		}
	}
}

void UItemContainerExtensionGroup::ClearParentGroup()
{
	if (IsValid(ParentGroup))
	{
		for (auto&& Container : Containers)
		{
			if (Container.IsValid())
			{
				ParentGroup->DeinitializeExtension(Container.Get());
			}
		}
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ParentGroup, this);
		ParentGroup = nullptr;
	}
}

void UItemContainerExtensionGroup::SetUnclaimedExtensionData(const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData)
{
	UnclaimedExtensionData = ExtensionData;
}

void UItemContainerExtensionGroup::TryApplyUnclaimedSaveData(UItemContainerExtensionBase* Extension)
{
	if (!UnclaimedExtensionData.IsValid())
	{
		return;
	}

	const FGuid ExtensionIdentifier = Extension->Identifier;
	if (!ensure(ExtensionIdentifier.IsValid())) return;
	const uint32 ExtensionHash = GetTypeHash(ExtensionIdentifier);

	for (TWeakObjectPtr<const UFaerieItemContainerBase> Container : Containers)
	{
		uint32 ContainerHash = GetTypeHash(Container->GetName());

		// Unique hash for the combo of this extension + container.
		const uint32 SaveHash = HashCombine(ExtensionHash, ContainerHash);

		if (auto&& SaveData = UnclaimedExtensionData.Get().Data.Find(SaveHash))
		{
			Extension->LoadSaveData(Container.Get(), *SaveData);
			UnclaimedExtensionData.Get().Data.Remove(SaveHash);
		}
	}
}

void UItemContainerExtensionGroup::ReplicationFixup()
{
	Utils::ClearLoadFlags(this);
	for (auto&& Extension : Extensions::FRecursiveExtensionIterator(this))
	{
		Utils::ClearLoadFlags(Extension);
	}
}

void UItemContainerExtensionGroup::ValidateGroup()
{
	if (!Identifier.IsValid())
	{
		UE_LOGF(LogFaerieInventory, Warning, "Invalid extension identifier for '%ls'", *GetName())
	}

	for (auto&& It = Extensions.CreateIterator(); It; ++It)
	{
		auto&& Extension = *It;

		if (!IsValid(Extension))
		{
			UE_LOGF(LogFaerieInventory, Warning,
				"Removing invalid extension pointer during PostLoadFixup at index [%i] for '%ls'",
				It.GetIndex(), *GetName())
			It.RemoveCurrent();
		}
		if (UItemContainerExtensionGroup* AsGroup = Cast<UItemContainerExtensionGroup>(Extension))
		{
			AsGroup->ValidateGroup();
		}
		else if (!Extension->Identifier.IsValid())
		{
			UE_LOGF(LogFaerieInventory, Warning, "Invalid extension identifier for '%ls'", *Extension->GetName())
		}
	}
}

void Extensions::FGroupAPI::PostEvent(GroupParam Group, ContainerParam Container,
	const Inventory::FEventData& Event, const FFaerieInventoryTag Reason)
{
	const Inventory::FEventLogBatch Batch(MakeConstArrayView(&Event, 1), Reason);
	Group->PostEventBatch(Container, Batch);
}

#undef LOCTEXT_NAMESPACE
