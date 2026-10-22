// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataLibrary.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemProxy.h"
#include "Fragments/FaerieAssetInfo.h"
#include "Fragments/ModificationDateFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataLibrary)

using namespace Faerie;

bool UFaerieItemDataLibrary::IsItemMutable(const FFaerieItemProxy& Proxy)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::GetItemLastModified"), ELogVerbosity::Error);
		return false;
	}

	return InstanceOpt.GetValue().IsMutable();
}

FDateTime UFaerieItemDataLibrary::GetItemLastModified(const FFaerieItemProxy& Proxy)
{
	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::GetItemLastModified"), ELogVerbosity::Error);
		return FDateTime();
	}

	const TSharedPtr<FMassEntityManager> EntityManager = Proxy.GetEntityManager();
	if (!EntityManager)
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieItemDataLibrary::GetItemLastModified"), ELogVerbosity::Error);
		return FDateTime();
	}

	if (auto* ModificationData = Faerie::ItemData::GetEntityFragment<FFaerieItemModificationDate>(*EntityManager, InstanceOpt->GetMassEntityHandle()))
	{
		return ModificationData->LastModified;
	}
	return FDateTime();
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

FFaerieUnownedItemStack UFaerieItemDataLibrary::NewItemInstance(const UObject* WorldContextObj, TArray<FInstancedStruct>& Fragments)
{
	for (int32 i = 0; i < Fragments.Num(); ++i)
	{
		if (!Fragments[i].IsValid())
		{
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("Invalid Fragment[%i] passed to UFaerieItemDataLibrary::NewItemInstance"), i), ELogVerbosity::Error);
			return FFaerieUnownedItemStack();
		}
	}

	const UWorld* World = WorldContextObj->GetWorld();
	if (!ItemData::HasFaerieEntityManagerBeenAssigned(World))
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieItemDataLibrary::NewItemInstance"), ELogVerbosity::Error);
		return FFaerieUnownedItemStack();
	}

	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);

	FFaerieUnownedItemStack OutStack;
	OutStack.Instance = FFaerieItemInstance::FromEntity(ItemData::InitializeItemEntity(EntityManager, nullptr, FMassTagBitSet(), Fragments));
	OutStack.Copies = 1;
	return OutStack;
}

bool UFaerieItemDataLibrary::HasItemFragment(const FFaerieItemProxy& Proxy, UScriptStruct* FragmentType)
{
	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::HasItemFragment"), ELogVerbosity::Error);
		return false;
	}

	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::HasItemFragment"), ELogVerbosity::Error);
		return false;
	}

	const TSharedPtr<FMassEntityManager> EntityManager = Proxy.GetEntityManager();
	return ItemData::HasEntityFragmentOrDefault(EntityManager.Get(), InstanceOpt.GetValue(), FragmentType);
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

bool UFaerieItemDataLibrary::FindFragment(const FFaerieItemProxy& Proxy, UScriptStruct* FragmentType,
	FInstancedStruct& FoundFragment)
{
	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::FindFragment"), ELogVerbosity::Error);
		return false;
	}

	const TOptional<FFaerieItemInstance> InstanceOpt = Proxy.GetItemInstance();
	if (!InstanceOpt.IsSet())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::FindFragment"), ELogVerbosity::Error);
		return false;
	}

	const TSharedPtr<FMassEntityManager> EntityManager = Proxy.GetEntityManager();
	FoundFragment = ItemData::GetEntityFragmentOrDefault(EntityManager.Get(), InstanceOpt.GetValue(), FragmentType);
	return FoundFragment.IsValid();
}

int32 UFaerieItemDataLibrary::UnlimitedStack()
{
	return ItemData::UnlimitedStack;
}

bool UFaerieItemDataLibrary::IsUnlimited(const int32 Stack)
{
	return Stack == ItemData::UnlimitedStack;
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
	const UObject* Obj = ProxyObject.GetObject();
	FMassEntityManager* EntityManager = ItemData::GetFaerieEntityManager(Obj->GetWorld());
	return FFaerieItemProxy(EntityManager ? EntityManager->AsShared().ToSharedPtr() : nullptr, ProxyObject);
}

const UObject* UFaerieItemDataLibrary::GetProxyObject(const FFaerieItemProxy& Proxy)
{
	return Proxy.GetProxyObject();
}

bool UFaerieItemDataLibrary::IsValid_ItemProxy(const FFaerieItemProxy& Proxy)
{
	return Proxy.HasValidInstance();
}

int32 UFaerieItemDataLibrary::GetProxyCopies(const FFaerieItemProxy& Proxy)
{
	return Proxy.GetCopies();
}

bool UFaerieItemDataLibrary::ItemIsMutablePredicate(const FFaerieItemProxy& Proxy)
{
	return Proxy.GetItemInstanceOrInvalid().IsMutable();
}

bool UFaerieItemDataLibrary::ItemIsImmutablePredicate(const FFaerieItemProxy& Proxy)
{
	return !Proxy.GetItemInstanceOrInvalid().IsMutable();
}

bool UFaerieItemDataLibrary::ItemLexicographicNameComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB)
{
	const TOptional<FFaerieItemInstance> ItemInstanceOptA = ProxyA.GetItemInstance();
	const TOptional<FFaerieItemInstance> ItemInstanceOptB = ProxyB.GetItemInstance();
	if (!ItemInstanceOptA.IsSet() || !ItemInstanceOptB.IsSet()) return false;

	FMassEntityManager* EntityManager = ProxyA.GetEntityManager().Get();
	auto InfoA = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ItemInstanceOptA.GetValue());
	auto InfoB = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieAssetInfo>(EntityManager, ItemInstanceOptB.GetValue());

	if (InfoA.IsValid() && InfoB.IsValid())
	{
		return InfoA->ObjectName.CompareTo(InfoB->ObjectName) <= 0;
	}

	return false;
}

bool UFaerieItemDataLibrary::ItemDateModifiedComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB)
{
	const TOptional<FFaerieItemInstance> ItemInstanceOptA = ProxyA.GetItemInstance();
	const TOptional<FFaerieItemInstance> ItemInstanceOptB = ProxyB.GetItemInstance();
	if (!ItemInstanceOptA.IsSet() || !ItemInstanceOptB.IsSet()) return false;

	if (const FMassEntityManager* EntityManager = ProxyA.GetEntityManager().Get())
	{
		const FFaerieItemModificationDate* ModificationDataA = Faerie::ItemData::GetEntityFragment<FFaerieItemModificationDate>(*EntityManager, ItemInstanceOptA.GetValue().GetMassEntityHandle());
		const FFaerieItemModificationDate* ModificationDataB = Faerie::ItemData::GetEntityFragment<FFaerieItemModificationDate>(*EntityManager, ItemInstanceOptB.GetValue().GetMassEntityHandle());

		const FDateTime DataA = ModificationDataA ? ModificationDataA->LastModified : FDateTime();
		const FDateTime DataB = ModificationDataB ? ModificationDataB->LastModified : FDateTime();

		return DataA < DataB;
	}
	return false;
}
