// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemContainerExtensionBase.h"
#include "FaerieItemContainerBase.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "FaerieUtils.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerExtensionBase)

#define LOCTEXT_NAMESPACE "ItemContainerExtensionGroup"

void UItemContainerExtensionBase::PostDuplicate(const EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	if (DuplicateMode == EDuplicateMode::PIE) return;

	// Make a new identifier when duplicated
	SetIdentifier();
}

void UItemContainerExtensionBase::InitializeNetObject(AActor* Actor)
{
	Super::InitializeNetObject(Actor);

	ensureAlwaysMsgf(!Faerie::HasLoadFlag(this),
		TEXT("Extensions must not be assets loaded from disk. (DuplicateObjectFromDiskForReplication or ClearLoadFlags can fix this)"
			LINE_TERMINATOR
			"	Failing Extension: '%s'"), *GetFullName());
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

void UItemContainerExtensionGroup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Extensions, Params);
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

void UItemContainerExtensionGroup::InitializeNetObject(AActor* Actor)
{
	Super::InitializeNetObject(Actor);

	ForEachExtension(
		[Actor](UItemContainerExtensionBase* Extension)
		{
			Actor->AddReplicatedSubObject(Extension);
			Extension->InitializeNetObject(Actor);
		});
}

void UItemContainerExtensionGroup::DeinitializeNetObject(AActor* Actor)
{
	ForEachExtension(
		[Actor](UItemContainerExtensionBase* Extension)
		{
			Actor->RemoveReplicatedSubObject(Extension);
			Extension->DeinitializeNetObject(Actor);
		});
}

void UItemContainerExtensionGroup::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;
	if (Containers.Contains(Container)) return;

#if WITH_EDITOR
	checkf(FUObjectThreadContext::Get().IsInConstructor == false, TEXT("Do not call InitializeExtension from a constructor! Use InitializeNetObject if available."));

	// Explanation: For UFaerieInventoryComponents that are added to Blueprint Classes, sometimes in PIE, the
	// component's GEN_VARIABLE version will somehow slip through and try to be added as a container. They are invalid.
	if (Container->GetFullName().Contains("GEN_VARIABLE"))
	{
		//ensure(0);
		return;
	}
#endif

	Containers.Emplace(Container);

	ForEachExtension(
		[Container](UItemContainerExtensionBase* Extension)
		{
			Extension->InitializeExtension(Container);
		});
}

void UItemContainerExtensionGroup::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;
	if (!Containers.Contains(Container)) return;

	ForEachExtension(
		[Container](UItemContainerExtensionBase* Extension)
		{
			Extension->DeinitializeExtension(Container);
		});

	Containers.Remove(Container);
}

EEventExtensionResponse UItemContainerExtensionGroup::AllowsAddition(const UFaerieItemContainerBase* Container,
																	 const TConstArrayView<FFaerieItemStackView> Views,
																	 const FFaerieExtensionAllowsAdditionArgs Args) const
{
	EEventExtensionResponse Response = EEventExtensionResponse::NoExplicitResponse;

	// Check each extension, to see if the reason is allowed or denied.
	for (auto&& Extension : Extensions)
	{
		if (!ensure(IsValid(Extension))) continue;

		switch (Extension->AllowsAddition(Container, Views, Args))
		{
		case EEventExtensionResponse::NoExplicitResponse:
			break;
		case EEventExtensionResponse::Allowed:
			// Flag response as allowed, unless another extension bars with a Disallowed
			Response = EEventExtensionResponse::Allowed;
			break;
		case EEventExtensionResponse::Disallowed:
			// Return false immediately if any Extension bars the reason.
			return EEventExtensionResponse::Disallowed;
		default: ;
		}
	}

	return Response;
}

void UItemContainerExtensionGroup::PreAddition(const UFaerieItemContainerBase* Container, const FFaerieItemStackView Stack)
{
	ForEachExtension(
		[Container, Stack](UItemContainerExtensionBase* Extension)
		{
			Extension->PreAddition(Container, Stack);
		});
}

void UItemContainerExtensionGroup::PostAddition(const UFaerieItemContainerBase* Container,
												const Faerie::Inventory::FEventLog& Event)
{
	ForEachExtension(
		[Container, &Event](UItemContainerExtensionBase* Extension)
		{
			Extension->PostAddition(Container, Event);
		});
}

EEventExtensionResponse UItemContainerExtensionGroup::AllowsRemoval(const UFaerieItemContainerBase* Container,
																	const FEntryKey Key, const FFaerieInventoryTag Reason) const
{
	EEventExtensionResponse Response = EEventExtensionResponse::NoExplicitResponse;

	// Check each extension, to see if the reason is allowed or denied.
	for (auto&& Extension : Extensions)
	{
		if (!ensure(IsValid(Extension))) continue;

		switch (Extension->AllowsRemoval(Container, Key, Reason))
		{
		case EEventExtensionResponse::NoExplicitResponse:
			break;
		case EEventExtensionResponse::Allowed:
			// Flag response as allowed, unless another extension bars with a Disallowed
			Response = EEventExtensionResponse::Allowed;
			break;
		case EEventExtensionResponse::Disallowed:
			// Return false immediately if any Extension bars the reason.
			return EEventExtensionResponse::Disallowed;
		default: ;
		}
	}

	return Response;
}

void UItemContainerExtensionGroup::PreRemoval(const UFaerieItemContainerBase* Container, const FEntryKey Key, const int32 Removal)
{
	ForEachExtension(
		[Container, Key, Removal](UItemContainerExtensionBase* Extension)
		{
			Extension->PreRemoval(Container, Key, Removal);
		});
}

void UItemContainerExtensionGroup::PostRemoval(const UFaerieItemContainerBase* Container,
                                           const Faerie::Inventory::FEventLog& Event)
{
	ForEachExtension(
		[Container, &Event](UItemContainerExtensionBase* Extension)
		{
			Extension->PostRemoval(Container, Event);
		});
}

EEventExtensionResponse UItemContainerExtensionGroup::AllowsEdit(const UFaerieItemContainerBase* Container,
																 const FEntryKey Key,
																 const FFaerieInventoryTag EditTag) const
{
	EEventExtensionResponse Response = EEventExtensionResponse::NoExplicitResponse;

	// Check each extension, to see if the edit tag is allowed or denied.
	for (auto&& Extension : Extensions)
	{
		if (!ensure(IsValid(Extension))) continue;

		switch (Extension->AllowsEdit(Container, Key, EditTag))
		{
		case EEventExtensionResponse::NoExplicitResponse:
			break;
		case EEventExtensionResponse::Allowed:
			// Flag response as allowed, unless another extension bars with a Disallowed
				Response = EEventExtensionResponse::Allowed;
			break;
		case EEventExtensionResponse::Disallowed:
			// Return false immediately if any Extension bars the reason.
				return EEventExtensionResponse::Disallowed;
		default: ;
		}
	}

	return Response;
}

void UItemContainerExtensionGroup::PostEntryChanged(const UFaerieItemContainerBase* Container,
	const Faerie::Inventory::FEventLog& Event)
{
	ForEachExtension(
		[Container, &Event](UItemContainerExtensionBase* Extension)
		{
			Extension->PostEntryChanged(Container, Event);
		});
}

UItemContainerExtensionGroup* UItemContainerExtensionGroup::GetExtensionGroup() const
{
	return const_cast<UItemContainerExtensionGroup*>(this);
}

void UItemContainerExtensionGroup::ForEachExtension(const TFunctionRef<void(UItemContainerExtensionBase*)>& Func)
{
	for (auto&& Extension : Extensions)
	{
		if (!ensureMsgf(IsValid(Extension), TEXT("Invalid Extension while iterating. Investigate!"))) continue;
		Func(Extension);
	}
}

#if !UE_BUILD_SHIPPING

namespace Helper
{
	// Copied from GeometryCollection/GeometryCollectionComponent.h
	// For some reason this isn't anywhere else in the engine :/
	FString RoleToString(const ENetRole InRole)
	{
		switch (InRole)
		{
		case ROLE_None:
			return FString(TEXT("None"));
		case ROLE_SimulatedProxy:
			return FString(TEXT("SimProxy"));
		case ROLE_AutonomousProxy:
			return FString(TEXT("AutoProxy"));
		case ROLE_Authority:
			return FString(TEXT("Auth"));
		default:
			break;
		}

		return FString(TEXT("Invalid Role"));
	}
}

void UItemContainerExtensionGroup::PrintDebugData() const
{
	const AActor* OwningActor = GetTypedOuter<AActor>();

	UE_LOG(LogTemp, Log, TEXT("Printing Containers/Extension in Group (%s)"),
		OwningActor ? *("Role: " + Helper::RoleToString(OwningActor->GetLocalRole())) : TEXT("No Owner"))
	for (auto&& Container : Containers)
	{
		if (!Container.IsValid())
		{
			UE_LOG(LogTemp, Warning,  TEXT("	Invalid Containers in PrintDebugData. Investigate!"))
			continue;
		}
		UE_LOG(LogTemp, Warning,  TEXT("	Registered Container: '%s'"), *Container->GetName())
	}

	for (auto&& Extension : Extensions)
	{
		if (!IsValid(Extension))
		{
			UE_LOG(LogTemp, Warning,  TEXT("	Invalid Extension in PrintDebugData. Investigate!"))
			continue;
		}
		UE_LOG(LogTemp, Warning,  TEXT("	Registered Extension: '%s'"), *Extension->GetName())
	}
}
#endif

bool UItemContainerExtensionGroup::AddExtension(UItemContainerExtensionBase* Extension)
{
	checkf(Extension->GetIdentifier().IsValid(),
		TEXT("Extension with invalid Identifier. Setup code-path with SetIdentifier called before AddExtension"))
	if (!ensure(IsValid(Extension))) return false;

	if (!ensureAlwaysMsgf(!Extensions.Contains(Extension), TEXT("Trying to add Extension twice. This is bad. Track down why!")))
	{
		return false;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Extensions, this);
	Extensions.Add(Extension);
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
	if (!Extensions.Contains(Extension)) return false;

	for (auto&& Container : Containers)
	{
		if (Container.IsValid())
		{
			Extension->DeinitializeExtension(Container.Get());
		}
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Extensions, this);
	return !!Extensions.Remove(Extension);
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

	for (auto&& Extension : Extensions)
	{
		if (!Extension) continue;

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

	for (auto&& Extension : Extensions)
	{
		if (!Extension) continue;

		// Find extension by direct search
		if (Extension->IsA(ExtensionClass))
		{
			return Extension;
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

void UItemContainerExtensionGroup::ReplicationFixup()
{
	Faerie::ClearLoadFlags(this);
	ForEachExtension([](UItemContainerExtensionBase* Extension)
	{
		if (UItemContainerExtensionGroup* Group = Cast<UItemContainerExtensionGroup>(Extension))
		{
			Group->ReplicationFixup();
		}
		else
		{
			Faerie::ClearLoadFlags(Extension);
		}
	});
}

#undef LOCTEXT_NAMESPACE
