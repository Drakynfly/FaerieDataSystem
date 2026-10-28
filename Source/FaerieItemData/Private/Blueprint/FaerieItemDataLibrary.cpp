// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataLibrary.h"
#include "BasicItemFilters.h"
#include "EntityManagerHelpers.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemInstance.h"
#include "FaerieItemProxy.h"
#include "Fragments/FaerieAssetInfo.h"
#include "Fragments/ModificationDateFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataLibrary)

using namespace Faerie;

bool UFaerieItemDataLibrary::IsItemMutable(const FFaerieItemProxy& Proxy)
{
	return Proxy.IsMutable();
}

FDateTime UFaerieItemDataLibrary::GetItemLastModified(const FFaerieItemProxy& Proxy)
{
	if (!Proxy.HasValidStack())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::GetItemLastModified"), ELogVerbosity::Error);
		return FDateTime();
	}

	if (const FFaerieItemModificationDate* ModificationData = Faerie::ItemData::GetEntityFragment<FFaerieItemModificationDate>(Proxy))
	{
		return ModificationData->LastModified;
	}
	return FDateTime();
}

const UFaerieItem* UFaerieItemDataLibrary::GetTemplateInstance(const UFaerieItemAsset* Asset)
{
	if (!Asset)
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Asset passed to UFaerieItemDataLibrary::GetTemplateInstance"), ELogVerbosity::Error);
		return nullptr;
	}

	return Asset->GetAssetTemplateItem();
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

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);

	FFaerieUnownedItemStack OutStack
	{
		ItemData::InitializeItemEntity(EntityManager, nullptr, FMassTagBitSet(), Fragments),
		1
	};
	return OutStack;
}

bool UFaerieItemDataLibrary::HasItemFragment(const FFaerieItemProxy& Proxy, UScriptStruct* FragmentType)
{
	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::HasItemFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Proxy.HasValidStack())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::HasItemFragment"), ELogVerbosity::Error);
		return false;
	}

	return ItemData::HasEntityFragmentOrDefault(Proxy, FragmentType);
}

bool UFaerieItemDataLibrary::FindFragment(const FFaerieItemProxy& Proxy, UScriptStruct* FragmentType,
	FInstancedStruct& FoundFragment)
{
	if (!IsValid(FragmentType))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid FragmentType passed to UFaerieItemDataLibrary::FindFragment"), ELogVerbosity::Error);
		return false;
	}

	if (!Proxy.HasValidStack())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Proxy passed to UFaerieItemDataLibrary::FindFragment"), ELogVerbosity::Error);
		return false;
	}

	FoundFragment = ItemData::GetEntityFragmentOrDefault(Proxy, FragmentType);
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
	return Proxy.HasValidStack();
}

int32 UFaerieItemDataLibrary::GetProxyCopies(const FFaerieItemProxy& Proxy)
{
	return Proxy.GetCopies();
}

bool UFaerieItemDataLibrary::ItemIsMutablePredicate(const FFaerieItemProxy& Proxy)
{
	FFaerieItemFilter_Mutability Filter;
	Filter.RequireMutable = true;
	return Filter.Exec(Proxy);
}

bool UFaerieItemDataLibrary::ItemIsImmutablePredicate(const FFaerieItemProxy& Proxy)
{
	FFaerieItemFilter_Mutability Filter;
	Filter.RequireMutable = false;
	return Filter.Exec(Proxy);
}

bool UFaerieItemDataLibrary::ItemLexicographicNameComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB)
{
	auto InfoA = Faerie::ItemData::GetItemDataOrDefault<FFaerieAssetInfo>(ProxyA);
	auto InfoB = Faerie::ItemData::GetItemDataOrDefault<FFaerieAssetInfo>(ProxyB);

	if (InfoA.IsValid() && InfoB.IsValid())
	{
		return InfoA->ObjectName.CompareTo(InfoB->ObjectName) <= 0;
	}

	return false;
}

bool UFaerieItemDataLibrary::ItemDateModifiedComparator(const FFaerieItemProxy& ProxyA, const FFaerieItemProxy& ProxyB)
{
	const FFaerieItemModificationDate* ModificationDataA = ItemData::GetEntityFragment<FFaerieItemModificationDate>(ProxyA);
	const FFaerieItemModificationDate* ModificationDataB = ItemData::GetEntityFragment<FFaerieItemModificationDate>(ProxyB);
	const FDateTime DataA = ModificationDataA ? ModificationDataA->LastModified : FDateTime();
	const FDateTime DataB = ModificationDataB ? ModificationDataB->LastModified : FDateTime();
	return DataA < DataB;
}
