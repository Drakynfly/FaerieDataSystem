// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GenerationAction_UpgradeItems.h"
#include "FaerieItemMutator.h"
#include "ItemUpgradeConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GenerationAction_UpgradeItems)

void UGenerationAction_UpgradeItems::Configure(FActionArgs& Args)
{
	UpgradeConfig = Args.UpgradeConfig;
	ItemBeingUpgraded = Args.ItemBeingUpgraded;
	check(UpgradeConfig);
	check(IsValid(ItemBeingUpgraded.GetObject()));
	Super::Configure(Args);
}

TArray<FSoftObjectPath> UGenerationAction_UpgradeItems::GetAssetsToLoad() const
{
	TArray<FSoftObjectPath> Paths;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	UpgradeConfig->Mutator->GetRequiredAssets(RequiredAssets);

	for (auto&& RequiredAsset : RequiredAssets)
	{
		Paths.Add(RequiredAsset.ToSoftObjectPath());
	}

	return Paths;
}

void UGenerationAction_UpgradeItems::Run()
{
	// Execute parent Run, as it validates some stuff, and then early out if it fails.
	Super::Run();
	if (!IsRunning()) return;

	// @todo batching
	int32 Copies = 1;

	const FFaerieItemStackView ReleaseRequest{ItemBeingUpgraded->GetItemObject(), Copies};
	const FFaerieItemStack Stack = ItemBeingUpgraded->GetItemOwner()->Release(ReleaseRequest);

	if (Stack.Copies == 0)
	{
		return Fail();
	}

	// Apply the mutator
	if (!UpgradeConfig->Mutator->TryApply(Stack))
	{
		return Fail();
	}

	ProcessStacks.Add(Stack);

	if (RunConsumeStep)
	{
		if (UpgradeConfig->Mutator->Implements<UFaerieItemSlotInterface>())
		{
			ConsumeSlotCosts(Cast<IFaerieItemSlotInterface>(UpgradeConfig->Mutator));
		}
	}

	Complete();
}