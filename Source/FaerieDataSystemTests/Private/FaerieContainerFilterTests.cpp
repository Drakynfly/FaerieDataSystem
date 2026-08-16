// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FaerieContainerFilter.h"
#include "FaerieItemStorage.h"

#include "Fragments/FaerieAssetInfo.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FaerieContainerFilterTests, "FDS.FaerieContainerFilterTests", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FaerieContainerFilterTests::RunTest(const FString& Parameters)
{
	using namespace Faerie::Container;

	static FFaerieAssetInfo TestInfo{
		.ObjectName = FText::FromString(TEXT("TestObjectName")),
		.ShortDescription = FText::FromString(TEXT("TestObjectShortDescription")),
		.LongDescription = FText::FromString(TEXT("TestObjectLongDescription")),
	};

	FInstancedStruct TestInfoInstance;
	TestInfoInstance.InitializeAs<FFaerieAssetInfo>(TestInfo);

	// Create an immutable item
	FFaerieItemInstance TestItem1 = FFaerieItemInstance::FromPointer(
		UFaerieItem::CreateNewInstance(MakeConstArrayView(&TestInfoInstance, 1), GetTransientPackageAsObject(), EFaerieItemInstancingMutability::Immutable));

	// Create a mutable item
	FFaerieItemInstance TestItem2 = FFaerieItemInstance::FromPointer(
		UFaerieItem::CreateNewInstance(MakeConstArrayView(&TestInfoInstance, 1), GetTransientPackageAsObject(), EFaerieItemInstancingMutability::Mutable));

	auto TestItem1Info = Faerie::ItemData::GetDefaultFragment<FFaerieAssetInfo>(TestItem1.GetItemPtr());

	TestTrue("GetDefaultFragment", TestItem1Info.IsValid());
	TestTrue("GetDefaultFragment equals TestInfo", TestItem1Info->ObjectName.EqualTo(TestInfo.ObjectName));

	UFaerieItemStorage* Storage = NewObject<UFaerieItemStorage>();

	Storage->AddEntryFromInstance(TestItem1, EFaerieStorageAddStackBehavior::AddToAnyStack);
	Storage->AddEntryFromInstance(TestItem2, EFaerieStorageAddStackBehavior::AddToAnyStack);
	int32 ExpectedEntries = 2;

	TestTrue("NumAfter2Adds", Storage->GetEntryCount() == ExpectedEntries);

	// Adding another immutable item should not create an entry
	Storage->AddEntryFromInstance(TestItem1, EFaerieStorageAddStackBehavior::AddToAnyStack);

	TestTrue("NumAfter3Adds", Storage->GetEntryCount() == ExpectedEntries);

	// Adding another mutable item should create an entry
	Storage->AddEntryFromInstance(TestItem2, EFaerieStorageAddStackBehavior::AddToAnyStack);
	ExpectedEntries++;

	TestTrue("NumAfter4Adds", Storage->GetEntryCount() == ExpectedEntries);

	FObjectKey TestItem1Key(TestItem1.GetItemPtr());

	AddInfo("Running Interface Tests...");

	// Test Interface Filter
	{
		auto ItemFilter = FItemFilter();

		TestTrue("(Interface) FilterNumIsExpected", ItemFilter.Count(nullptr, Storage) == ExpectedEntries);

		{
			TFilter<EFilterFlags::Inverted, const UFaerieItem*, IEntryIterator> InvertedFilter = ItemFilter.Invert();

			TestTrue("(Interface) FilterNumIs0", InvertedFilter.Count(nullptr, Storage) == 0);

			TFilter<EFilterFlags::None, const UFaerieItem*, IEntryIterator> DoubleInvertedFilter = InvertedFilter.Invert();

			TestTrue("(Interface) FilterNumIsExpectedAgain", DoubleInvertedFilter.Count(nullptr, Storage) == ExpectedEntries);
		}

		FObjectKey ItemFromFilter = ItemFilter.First(nullptr, Storage);
		TestTrue("(Interface) IteratorItem resolved to item", ItemFromFilter == TestItem1Key);

		int32 RangeCount = 0;
		for (auto It = ItemFilter.Iterate(nullptr, Storage); It; ++It)
		{
			TestTrue("(Interface) Iteration element is valid", IsValid(*It));
			++RangeCount;
		}
		if (!TestTrue("(Interface) Range count matched Entries", RangeCount == ExpectedEntries))
		{
			AddInfo("RangeCount: " + LexToString(RangeCount) + ", Expected: " + LexToString(ExpectedEntries));
		}
	}

	return true;
}

#endif