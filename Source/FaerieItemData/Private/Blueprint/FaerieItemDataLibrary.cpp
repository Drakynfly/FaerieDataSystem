// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataLibrary.h"
#include "DelegateCommon.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemDataView.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"

#include "Fragments/FaerieAssetInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataLibrary)

bool UFaerieItemDataLibrary::IsValid_ItemInstance(const FFaerieItemInstance& Instance)
{
	return Instance.IsValid();
}

bool UFaerieItemDataLibrary::EqualEqual_ItemInstance(const FFaerieItemInstance& A, const FFaerieItemInstance& B)
{
	return A == B;
}

bool UFaerieItemDataLibrary::IsItemMutable(const FFaerieItemInstance& Item)
{
	return Item.IsMutable();
}

FDateTime UFaerieItemDataLibrary::GetItemLastModified(const FFaerieItemInstance& Item)
{
	return Item.GetLastModified();
}

FFaerieItemInstance UFaerieItemDataLibrary::GetItemInstance(const UFaerieItemAsset* Asset)
{
	if (!Asset)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Asset passed to UFaerieItemDataLibrary::GetItemInstance"), ELogVerbosity::Error);
		return FFaerieItemInstance();
	}

	return Asset->GetTemplateInstance();
}

FFaerieItemInstance UFaerieItemDataLibrary::NewItemInstance(UObject* WorldContextObject, TArray<FInstancedStruct>& Fragments)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieItemDataLibrary::NewItemInstance"), ELogVerbosity::Error);
		return FFaerieItemInstance();
	}

	for (int32 i = 0; i < Fragments.Num(); ++i)
	{
		if (!Fragments[i].IsValid())
		{
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("Invalid Fragment[%i] passed to UFaerieItemDataLibrary::NewItemInstance"), i), ELogVerbosity::Error);
			return FFaerieItemInstance();
		}
	}

	CHECK_NOT_CALLED_IN_EDITOR(WorldContextObject)

	FFaerieItemInstance Instance = FFaerieItemInstance::FromFragments(Faerie::ItemData::GetFaerieEntityManagerChecked(), Fragments);
	Instance.InitializeMassEntity(Faerie::ItemData::GetFaerieEntityManagerChecked());
	return Instance;
}

bool UFaerieItemDataLibrary::HasItemFragment(UObject* WorldContextObject, const FFaerieItemInstance& Instance, UScriptStruct* FragmentType)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieItemDataLibrary::AddFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Instance.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Instance passed to UFaerieItemDataLibrary::AddFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::HasItemFragment"), ELogVerbosity::Error);
		return false;
	}

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObject);
	auto FoundFragment = Faerie::ItemData::GetEntityFragmentOrDefault(EntityManager, Instance, FragmentType);
	return FoundFragment.IsValid();
}

bool UFaerieItemDataLibrary::AddFragment(UObject* WorldContextObject, FFaerieItemInstance& Instance, FInstancedStruct Fragment)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieItemDataLibrary::RemoveFragment"), ELogVerbosity::Error);
		return false;
	}

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

	CHECK_NOT_CALLED_IN_EDITOR(WorldContextObject)

	Instance.AddFragment(Faerie::ItemData::GetFaerieEntityManagerChecked(), MoveTemp(Fragment));
	return true;
}

bool UFaerieItemDataLibrary::RemoveFragment(UObject* WorldContextObject, FFaerieItemInstance& Instance, const UScriptStruct* FragmentType)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieItemDataLibrary::RemoveFragment"), ELogVerbosity::Error);
		return false;
	}

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

	CHECK_NOT_CALLED_IN_EDITOR(WorldContextObject)

	Instance.RemoveFragment(Faerie::ItemData::GetFaerieEntityManagerChecked(), FragmentType);
	return true;
}

bool UFaerieItemDataLibrary::FindFragment(UObject* WorldContextObject, const FFaerieItemInstance& Instance,
	UScriptStruct* FragmentType, TInstancedStruct<FFaerieMassFragment>& FoundFragment)
{
	if (!IsValid(WorldContextObject))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObject passed to UFaerieItemDataLibrary::FindFragment"), ELogVerbosity::Error);
		return false;
	}

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

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObject);
	FoundFragment = Faerie::ItemData::GetEntityFragmentOrDefault(EntityManager, Instance, FragmentType);
	return FoundFragment.IsValid();
}

int32 UFaerieItemDataLibrary::UnlimitedStack()
{
	return Faerie::ItemData::UnlimitedStack;
}

bool UFaerieItemDataLibrary::IsUnlimited(const int32 Stack)
{
	return Stack == Faerie::ItemData::UnlimitedStack;
}

FFaerieItemInstance UFaerieItemDataLibrary::GetViewItem(const FFaerieItemDataView& View)
{
	if (!View.IsValid()) return FFaerieItemInstance();
	return View.GetInstance();
}

int32 UFaerieItemDataLibrary::GetViewCopies(const FFaerieItemDataView& View)
{
	if (!View.IsValid()) return 0;
	return View.GetCopies();
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieItemDataLibrary::GetViewOwner(const FFaerieItemDataView& View)
{
	if (!View.IsValid()) return nullptr;
	// Not const safe, but only so much we can do with BP the way it is...
	return const_cast<UObject*>(Cast<UObject>(View.GetOwner()));
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

FFaerieItemInstance UFaerieItemDataLibrary::GetProxyItemInstance(const FFaerieItemProxy& Proxy)
{
	if (!Proxy.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::GetProxyMassHandle"), ELogVerbosity::Error);
		return FFaerieItemInstance();
	}

	if (const TOptional<FFaerieItemInstance> Instance = Proxy->GetItemInstance();
		Instance.IsSet())
	{
		return Instance.GetValue();
	}

	return FFaerieItemInstance();
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieItemDataLibrary::GetProxyItemOwner(const FFaerieItemProxy& Proxy)
{
	if (Proxy.IsValid())
	{
		return Cast<UObject>(Proxy->GetItemOwner());
	}
	return nullptr;
}

int32 UFaerieItemDataLibrary::GetProxyCopies(const FFaerieItemProxy& Proxy)
{
	if (Proxy.IsValid())
	{
		return Proxy->GetCopies();
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

FFaerieItemDataView UFaerieItemDataLibrary::ProxyToView(const FFaerieItemProxy& Proxy)
{
	// Copy the proxy, so FaerieItemDataView can have one to own.
	return FFaerieItemProxy(Proxy);
}

bool UFaerieItemDataLibrary::ItemIsMutablePredicate(UObject*, const FFaerieItemDataView& View)
{
	return View.IsValid() && View.GetInstance().IsMutable();
}

bool UFaerieItemDataLibrary::ItemIsImmutablePredicate(UObject*, const FFaerieItemDataView& View)
{
	return View.IsValid() && !View.GetInstance().IsMutable();
}

bool UFaerieItemDataLibrary::ItemLexicographicNameComparator(UObject* WorldContextObj, const FFaerieItemDataView& ViewA, const FFaerieItemDataView& ViewB)
{
	if (!ViewA.IsValid() || !ViewB.IsValid()) return false;

	const Faerie::ItemData::FOptionalEntityManager EntityManager(WorldContextObj);
	auto InfoA = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ViewA.GetInstance());
	auto InfoB = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ViewB.GetInstance());

	if (InfoA.IsValid() && InfoB.IsValid())
	{
		return InfoA->ObjectName.CompareTo(InfoB->ObjectName) <= 0;
	}

	return false;
}

bool UFaerieItemDataLibrary::ItemDateModifiedComparator(UObject* WorldContextObj, const FFaerieItemDataView& ViewA, const FFaerieItemDataView& ViewB)
{
	if (!ViewA.IsValid() || !ViewB.IsValid()) return false;

	const FFaerieItemInstance ItemA = ViewA.GetInstance();
	const FFaerieItemInstance ItemB = ViewB.GetInstance();

	if (!(ItemA.IsValid() && ItemB.IsValid()))
	{
		return false;
	}

	return ItemA.GetLastModified() < ItemB.GetLastModified();
}
