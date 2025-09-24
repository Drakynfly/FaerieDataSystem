// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "DummyItemOwner.h"
#include "FaerieDataSystemTestsSettings.h"
#include "FaerieItem.h"
#include "FaerieItemAsset.h"
#include "FaerieItemDataEnums.h"
#include "FaerieItemStorageStatics.h"
#include "FaerieItemStack.h"
#include "FlakesInterface.h"
#include "Engine/AssetManager.h"
#include "Misc/AutomationTest.h"
#include "Providers/FlakesBinarySerializer.h"
#include "UObject/ReferencerFinder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FaerieItemSerializationTests,
								 "FDS.FaerieItemSerializationTests",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FaerieItemSerializationTests::RunTest(const FString& Parameters)
{
	const UFaerieDataSystemTestsSettings* Settings = GetDefault<UFaerieDataSystemTestsSettings>();
	TSoftObjectPtr<UFaerieItemAsset> SoftImmutableItemAsset(Settings->TestImmutableItemAsset);
	TSoftObjectPtr<UFaerieItemAsset> SoftMutableItemAsset(Settings->TestMutableItemAsset);

	FStreamableManager StreamableManager;


	{
		TestFalse(TEXT("Immutable item asset is not yet loaded1"), SoftImmutableItemAsset.IsValid());
		TestTrue(TEXT("Immutable item asset is not yet loaded2"), SoftImmutableItemAsset.IsPending());
		UFaerieItemAsset* ImmutableItemAsset = StreamableManager.LoadSynchronous<UFaerieItemAsset>(SoftImmutableItemAsset);
		TestTrue(TEXT("Settings has configured Immutable Asset"), IsValid(ImmutableItemAsset));

		// Instance the immutable asset. The instance should be immutable.
		const UFaerieItem* ImmutableItem = ImmutableItemAsset->GetItemInstance(EFaerieItemInstancingMutability::Automatic);
		TestFalse("Instance is instance immutable", ImmutableItem->IsInstanceMutable());
		TestFalse("Instance is data immutable", ImmutableItem->IsDataMutable());

		FFaerieItemStack ItemStack { ImmutableItem, 1 };

		const FFlake StackFlake = Flakes::MakeFlake<Flakes::Binary::Type>(ItemStack, nullptr);
		TestTrue(TEXT("Make Flake from immutable stack"), StackFlake.Struct.TryLoad() == FFaerieItemStack::StaticStruct());

		FFaerieItemStack ItemStack2 = Flakes::CreateStruct<Flakes::Binary::Type, FFaerieItemStack>(StackFlake);
		TestTrue(TEXT("Create Struct"), Faerie::ValidateItemData(ItemStack2.Item));

		TestTrue(TEXT("Immutable load is original"), ImmutableItem == ItemStack2.Item);

		ImmutableItem = nullptr;
		ItemStack = FFaerieItemStack();
		ItemStack2 = FFaerieItemStack();


		TArray<UObject*> Objs;
		Objs.Add(ImmutableItemAsset);
		TArray<UObject*> Reference = FReferencerFinder::GetAllReferencers(Objs,	nullptr,
			EReferencerFinderFlags::SkipInnerReferences);
		for (UObject* Object : Reference)
		{
			if (Object->GetClass()->GetFName() != TEXT("GCObjectReferencer"))
			{
				AddError(FString::Printf(TEXT("Referenced by: '%s'"), *Object->GetFullName()));
			}
		}

		ImmutableItemAsset = nullptr;

		StreamableManager.Unload(SoftImmutableItemAsset.ToSoftObjectPath());

		CollectGarbage(RF_NoFlags, true);

		TestFalse(TEXT("Immutable item asset is unloaded1"), SoftImmutableItemAsset.IsValid());
		TestTrue(TEXT("Immutable item asset is unloaded2"), SoftImmutableItemAsset.IsPending());

		FFaerieItemStack ItemStack3 = Flakes::CreateStruct<Flakes::Binary::Type, FFaerieItemStack>(StackFlake);
		TestTrue(TEXT("Create Struct after unload (immutable)"), Faerie::ValidateItemData(ItemStack3.Item));
	}

	// A dummy object to own the mutable items we instantiate.
	UObject* ItemOwner = NewObject<UDummyItemOwner>();
	ItemOwner->AddToRoot();

	{
		TestFalse(TEXT("Mutable item asset is not yet loaded1"), SoftMutableItemAsset.IsValid());
		TestTrue(TEXT("Mutable item asset is not yet loaded2"), SoftMutableItemAsset.IsPending());
		UFaerieItemAsset* MutableItemAsset = StreamableManager.LoadSynchronous<UFaerieItemAsset>(SoftMutableItemAsset);
		TestTrue(TEXT("Settings has configured Mutable Asset"), IsValid(MutableItemAsset));

		// Instance the mutable asset. The instance should be mutable.
		const UFaerieItem* MutableItem = MutableItemAsset->GetItemInstance(EFaerieItemInstancingMutability::Automatic);
		TestTrue("Instance is instance mutable", MutableItem->IsInstanceMutable());
		TestTrue("Instance is data mutable", MutableItem->IsDataMutable());
		MutableItem->MutateCast()->Rename(nullptr, ItemOwner, REN_DontCreateRedirectors);

		FFaerieItemStack ItemStack { MutableItem, 1 };

		const FFlake StackFlake_NoOuter = Flakes::MakeFlake<Flakes::Binary::Type>(ItemStack, nullptr);
		TestTrue(TEXT("Make Flake from mutable stack"), StackFlake_NoOuter.Struct.TryLoad() == FFaerieItemStack::StaticStruct());

		FFaerieItemStack ItemStackNoOuter = Flakes::CreateStruct<Flakes::Binary::Type, FFaerieItemStack>(StackFlake_NoOuter, nullptr);
		TestTrue(TEXT("Create Struct"), Faerie::ValidateItemData(ItemStackNoOuter.Item));

		// Item was serialized without an outer, should be the same item (as the original MutableItem)
		TestTrue(TEXT("Mutable load is not original when serialized with outer"), MutableItem == ItemStackNoOuter.Item);
		{
			AddInfo("MutableItem = " + MutableItem->GetFullName());
			AddInfo("Loaded item = " + ItemStackNoOuter.Item->GetFullName());
		}

		const FFlake StackFlake_WithOuter = Flakes::MakeFlake<Flakes::Binary::Type>(ItemStack, ItemOwner);
		TestTrue(TEXT("Make Flake from mutable stack"), StackFlake_WithOuter.Struct.TryLoad() == FFaerieItemStack::StaticStruct());

		FFaerieItemStack ItemStackWithOuter = Flakes::CreateStruct<Flakes::Binary::Type, FFaerieItemStack>(StackFlake_WithOuter, ItemOwner);
		TestTrue(TEXT("Create Struct"), Faerie::ValidateItemData(ItemStackWithOuter.Item));

		// Item was serialized with a valid outer, should be a new same item (not the original MutableItem)
		if (!TestFalse(TEXT("Mutable load is original when serialized without outer"), MutableItem == ItemStackWithOuter.Item))
		{
			AddInfo("MutableItem = " + MutableItem->GetFullName());
			AddInfo("Loaded item = " + ItemStackWithOuter.Item->GetFullName());
		}

		MutableItem = nullptr;
		ItemStack = FFaerieItemStack();
		ItemStackNoOuter = FFaerieItemStack();
		ItemStackWithOuter = FFaerieItemStack();


		TArray<UObject*> Objs;
		Objs.Add(MutableItemAsset);
		TArray<UObject*> Reference = FReferencerFinder::GetAllReferencers(Objs,	nullptr,
			EReferencerFinderFlags::SkipInnerReferences);
		for (UObject* Object : Reference)
		{
			if (Object->GetClass()->GetFName() != TEXT("GCObjectReferencer"))
			{
				AddError(FString::Printf(TEXT("Referenced by: '%s'"), *Object->GetFullName()));
			}
		}

		MutableItemAsset = nullptr;

		StreamableManager.Unload(SoftMutableItemAsset.ToSoftObjectPath());

		CollectGarbage(RF_NoFlags, true);

		TestFalse(TEXT("Mutable item asset is unloaded1"), SoftMutableItemAsset.IsValid());
		TestTrue(TEXT("Mutable item asset is unloaded2"), SoftMutableItemAsset.IsPending());

		// This item should not be able to load, as it was only exported as reference.
		FFaerieItemStack ItemStack3 = Flakes::CreateStruct<Flakes::Binary::Type, FFaerieItemStack>(StackFlake_NoOuter, ItemOwner);
		TestFalse(TEXT("Create Struct after unload failure"), IsValid(ItemStack3.Item));

		// This item should be able to load, as it was fully exported (due to having an outer)
		FFaerieItemStack ItemStack4 = Flakes::CreateStruct<Flakes::Binary::Type, FFaerieItemStack>(StackFlake_WithOuter, ItemOwner);
		TestTrue(TEXT("Create Struct after unload (mutable)"), Faerie::ValidateItemData(ItemStack4.Item));
	}

	ItemOwner->RemoveFromRoot();

	// Make the test pass by returning true, or fail by returning false.
	return true;
}

#endif