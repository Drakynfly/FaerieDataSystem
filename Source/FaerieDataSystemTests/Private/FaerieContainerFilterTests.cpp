// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FaerieContainerFilter.h"
#include "FaerieItemStorage.h"
#include "Tokens/FaerieInfoToken.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FaerieContainerFilterTests, "FDS.FaerieContainerFilterTests", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FaerieContainerFilterTests::RunTest(const FString& Parameters)
{
	using namespace Faerie::Container;

	static FFaerieAssetInfo TestInfo{
		FText::FromString(TEXT("TestObjectName")),
		FText::FromString(TEXT("TestObjectShortDescription")),
		FText::FromString(TEXT("TestObjectLongDescription")),
		nullptr
	};

	UFaerieItemToken* InfoToken1 = UFaerieInfoToken::CreateInstance(TestInfo);

	// Create an immutable item
	UFaerieItem* TestItem1 = UFaerieItem::CreateNewInstance(MakeArrayView(&InfoToken1, 1));

	// Create a mutable item
	UFaerieItem* TestItem2 = UFaerieItem::CreateNewInstance({}, EFaerieItemInstancingMutability::Mutable);
	TestItem2->AddToken(InfoToken1);

	TestTrue("GetToken (StaticClass)", TestItem1->GetOwnedToken(UFaerieInfoToken::StaticClass()) == InfoToken1);
	TestTrue("GetToken (Template)", TestItem1->GetOwnedToken<UFaerieInfoToken>() == InfoToken1);

	UFaerieItemStorage* Storage = NewObject<UFaerieItemStorage>();

	Storage->AddEntryFromItemObject(TestItem1, EFaerieStorageAddStackBehavior::AddToAnyStack);
	Storage->AddEntryFromItemObject(TestItem2, EFaerieStorageAddStackBehavior::AddToAnyStack);
	int32 ExpectedEntries = 2;

	TestTrue("NumAfter2Adds", Storage->GetEntryCount() == ExpectedEntries);

	// Adding another immutable item should not create an entry
	Storage->AddEntryFromItemObject(TestItem1, EFaerieStorageAddStackBehavior::AddToAnyStack);

	TestTrue("NumAfter3Adds", Storage->GetEntryCount() == ExpectedEntries);

	// Adding another mutable item should create an entry
	Storage->AddEntryFromItemObject(TestItem2, EFaerieStorageAddStackBehavior::AddToAnyStack);
	ExpectedEntries++;

	TestTrue("NumAfter4Adds", Storage->GetEntryCount() == ExpectedEntries);

	FObjectKey TestItem1Key(TestItem1);

	AddInfo("Running Interface Tests...");

	// Test Interface Filter
	{
		auto ItemFilter = FItemFilter();

		TestTrue("(Interface) FilterNumIsExpected", ItemFilter.Count(Storage) == ExpectedEntries);

		{
			TFilter<EFilterFlags::Inverted, const UFaerieItem*> InvertedFilter = ItemFilter.Invert();

			TestTrue("(Interface) FilterNumIs0", InvertedFilter.Count(Storage) == 0);

			TFilter<EFilterFlags::None, const UFaerieItem*> DoubleInvertedFilter = InvertedFilter.Invert();

			TestTrue("(Interface) FilterNumIsExpectedAgain", DoubleInvertedFilter.Count(Storage) == ExpectedEntries);
		}

		FObjectKey ItemFromFilter = ItemFilter.First(Storage);
		TestTrue("(Interface) IteratorItem resolved to item", ItemFromFilter == TestItem1Key);

		int32 RangeCount = 0;
		for (auto It = ItemFilter.Iterate(Storage); It; ++It)
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