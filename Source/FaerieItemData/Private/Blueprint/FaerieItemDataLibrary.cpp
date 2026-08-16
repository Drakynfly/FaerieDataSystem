// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataLibrary.h"
#include "DelegateCommon.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"
#include "Fragments/FaerieAssetInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataLibrary)

bool UFaerieItemDataLibrary::IsItemMutable(const FFaerieItemProxy& Proxy)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::GetItemLastModified"), ELogVerbosity::Error);
		return false;
	}

	auto InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return false;
	}

	return InstanceOpt.GetValue().IsMutable();
}

FDateTime UFaerieItemDataLibrary::GetItemLastModified(const FFaerieItemProxy& Proxy)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::GetItemLastModified"), ELogVerbosity::Error);
		return FDateTime();
	}

	auto InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return FDateTime();
	}

	return InstanceOpt.GetValue().GetLastModified();
}

FFaerieUnownedItemStack UFaerieItemDataLibrary::GetTemplateInstance(const UFaerieItemAsset* Asset)
{
	if (!Asset)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Asset passed to UFaerieItemDataLibrary::GetTemplateInstance"), ELogVerbosity::Error);
		return FFaerieUnownedItemStack();
	}

	return FFaerieUnownedItemStack(Asset->GetTemplateInstance(), 1);
}

FFaerieUnownedItemStack UFaerieItemDataLibrary::NewItemInstance(TArray<FInstancedStruct>& Fragments)
{
	for (int32 i = 0; i < Fragments.Num(); ++i)
	{
		if (!Fragments[i].IsValid())
		{
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("Invalid Fragment[%i] passed to UFaerieItemDataLibrary::NewItemInstance"), i), ELogVerbosity::Error);
			return FFaerieUnownedItemStack();
		}
	}

	if (!Faerie::ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieItemDataLibrary::NewItemInstance"), ELogVerbosity::Error);
		return FFaerieUnownedItemStack();
	}

	auto& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked();
	FFaerieItemInstance Instance = FFaerieItemInstance::FromFragments(EntityManager, Fragments);
	Instance.InitializeMassEntity(EntityManager);
	return FFaerieUnownedItemStack(Instance, 1);
}

bool UFaerieItemDataLibrary::HasItemFragment(const FFaerieItemProxy& Proxy, UScriptStruct* FragmentType)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::HasItemFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::HasItemFragment"), ELogVerbosity::Error);
		return false;
	}

	auto InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		return false;
	}

	auto FoundFragment = Faerie::ItemData::GetEntityFragmentOrDefault(Faerie::ItemData::GetFaerieEntityManager(), InstanceOpt.GetValue(), FragmentType);
	return FoundFragment.IsValid();
}

/*
bool UFaerieItemDataLibrary::AddFragment(FFaerieItemInstance& Instance, FInstancedStruct Fragment)
{
	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieItemDataLibrary::AddFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Fragment.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid fragment passed to UFaerieItemDataLibrary::AddFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Instance.IsMutable())
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot add fragment to immutable instance in UFaerieItemDataLibrary::AddFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Faerie::ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieItemDataLibrary::AddFragment"), ELogVerbosity::Error);
		return false;
	}

	Instance.AddFragment(Faerie::ItemData::GetFaerieEntityManagerChecked(), MoveTemp(Fragment));
	return true;
}
*/

/*
bool UFaerieItemDataLibrary::RemoveFragment(FFaerieItemInstance& Instance, const UScriptStruct* FragmentType)
{
	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieItemDataLibrary::RemoveFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::RemoveFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Instance.IsMutable())
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot remove fragment from immutable instance in UFaerieItemDataLibrary::RemoveFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Faerie::ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieItemDataLibrary::RemoveFragment"), ELogVerbosity::Error);
		return false;
	}

	Instance.RemoveFragment(Faerie::ItemData::GetFaerieEntityManagerChecked(), FragmentType);
	return true;
}
*/

bool UFaerieItemDataLibrary::FindFragment(const FFaerieItemInstance& Instance,
	UScriptStruct* FragmentType, TInstancedStruct<FFaerieMassFragment>& FoundFragment)
{
	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieItemDataLibrary::FindFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::FindFragment"), ELogVerbosity::Error);
		return false;
	}

	FoundFragment = Faerie::ItemData::GetEntityFragmentOrDefault(Faerie::ItemData::GetFaerieEntityManager(), Instance, FragmentType);
	return FoundFragment.IsValid();
}

bool UFaerieItemDataLibrary::FindFragment_Proxy(const FFaerieItemProxy& Proxy, UScriptStruct* FragmentType,
	TInstancedStruct<FFaerieMassFragment>& FoundFragment)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::FindFragment_Proxy"), ELogVerbosity::Error);
		return false;
	}

	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::FindFragment_Proxy"), ELogVerbosity::Error);
		return false;
	}

	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (InstanceOpt.IsSet())
	{
		FoundFragment = Faerie::ItemData::GetEntityFragmentOrDefault(Faerie::ItemData::GetFaerieEntityManager(), InstanceOpt.GetValue(), FragmentType);
		return FoundFragment.IsValid();
	}
	return false;
}

int32 UFaerieItemDataLibrary::UnlimitedStack()
{
	return Faerie::ItemData::UnlimitedStack;
}

bool UFaerieItemDataLibrary::IsUnlimited(const int32 Stack)
{
	return Stack == Faerie::ItemData::UnlimitedStack;
}

int32 UFaerieItemDataLibrary::GetViewCopies(const FFaerieItemProxy& Proxy)
{
	if (!Proxy.IsValid()) return 0;
	return Proxy.GetCopies();
}

bool UFaerieItemDataLibrary::EqualEqual_ItemProxy(const FFaerieItemProxy& A, const FFaerieItemProxy& B)
{
	return A == B;
}

bool UFaerieItemDataLibrary::NotEqual_ItemProxy(const FFaerieItemProxy& A, const FFaerieItemProxy& B)
{
	return A != B;
}

bool UFaerieItemDataLibrary::CastProxy(const FFaerieItemProxy& Proxy, UClass* Class, UObject*& ProxyObject)
{
	// @note The const_cast here is fine. This function acts as a "Resolve" of the weak pointer in the proxy struct,
	// which is usually treated as a const view of a proxy. CastProxy implicitly wants to resolve to a non-const pointer,
	// and Blueprint doesn't understand const pointers anyway.
	if (const UObject* Object = Proxy.GetProxyObject())
	{
		ProxyObject = const_cast<UObject*>(Object);
		return ProxyObject->IsA(Class);
	}
	return false;
}

FFaerieItemProxy UFaerieItemDataLibrary::ToProxyStruct(const TScriptInterface<IFaerieItemDataProxy>& ProxyObject)
{
	return FFaerieItemProxy(ProxyObject);
}

const UObject* UFaerieItemDataLibrary::GetProxyObject(const FFaerieItemProxy& Proxy)
{
	return Proxy.GetProxyObject();
}

bool UFaerieItemDataLibrary::IsValid_ItemProxy(const FFaerieItemProxy& Proxy)
{
	return Proxy.IsValid();
}

int32 UFaerieItemDataLibrary::GetProxyCopies(const FFaerieItemProxy& Proxy)
{
	if (Proxy.IsValid())
	{
		return Proxy.GetCopies();
	}
	return 0;
}

void UFaerieItemDataLibrary::BindToItemDataChanged(const FFaerieItemProxy& Proxy,
	const FFaerieItemProxyChangedEvent& Event)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::BindToItemDataChanged"), ELogVerbosity::Error);
		return;
	}

	(void)Proxy.GetOnProxyChangeEvent().Add(DYNAMIC_TO_NATIVE(Faerie::ItemData::FProxyChangeEvent::FDelegate, Event));
}

void UFaerieItemDataLibrary::UnbindAllFromItemDataChanged(const FFaerieItemProxy& Proxy, const UObject* Object)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::UnbindAllFromItemDataChanged"), ELogVerbosity::Error);
		return;
	}

	Proxy.GetOnProxyChangeEvent().RemoveAll(Object);
}

bool UFaerieItemDataLibrary::ItemIsMutablePredicate(const FFaerieItemProxy& Proxy)
{
	return Proxy.IsValid() && Proxy.GetItemInstanceOrInvalid().IsMutable();
}

bool UFaerieItemDataLibrary::ItemIsImmutablePredicate(const FFaerieItemProxy& Proxy)
{
	return Proxy.IsValid() && !Proxy.GetItemInstanceOrInvalid().IsMutable();
}

bool UFaerieItemDataLibrary::ItemLexicographicNameComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB)
{
	if (!ProxyA.IsValid() || !ProxyB.IsValid()) return false;

	auto* EntityManager = Faerie::ItemData::GetFaerieEntityManager();
	auto InfoA = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ProxyA.GetItemInstanceOrInvalid());
	auto InfoB = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ProxyB.GetItemInstanceOrInvalid());

	if (InfoA.IsValid() && InfoB.IsValid())
	{
		return InfoA->ObjectName.CompareTo(InfoB->ObjectName) <= 0;
	}

	return false;
}

bool UFaerieItemDataLibrary::ItemDateModifiedComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB)
{
	if (!ProxyA.IsValid() || !ProxyB.IsValid()) return false;

	const FFaerieItemInstance ItemA = ProxyA.GetItemInstanceOrInvalid();
	const FFaerieItemInstance ItemB = ProxyB.GetItemInstanceOrInvalid();

	if (!(ItemA.IsValid() && ItemB.IsValid()))
	{
		return false;
	}

	return ItemA.GetLastModified() < ItemB.GetLastModified();
}
