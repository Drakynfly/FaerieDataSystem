// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Subsystems/ConfigLoaderSubsystem.h"

#include "FaerieItemDataLog.h"
#include "FaerieItemDataSettings.h"
#include "MassEntityConfigAsset.h"

#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ConfigLoaderSubsystem)

void UFaerieMassConfigLoaderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	OverrideSubsystemTraits<ThisClass>(Collection);
}

void UFaerieMassConfigLoaderSubsystem::PostInitialize()
{
	Super::PostInitialize();

	const FSoftObjectPath MassConfigSoftObject = GetDefault<UFaerieItemDataSettings>()->ItemDataMassConfig.ToSoftObjectPath();
	if (MassConfigSoftObject.IsNull())
	{
		UE_LOGF(LogFaerieItemData, Error, "Invalid ItemDataMassConfig in ItemDataSettings! Please assign an asset to Project Settings -> Item Data Settings -> ItemDataMassConfig");
		return;
	}

	// Start to load the item data config asynchronously.
	ItemDataMassConfigStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MassConfigSoftObject,
		FStreamableDelegate::CreateUObject(this, &ThisClass::OnItemDataMassConfigLoaded));
}

const FMassEntityTemplate& UFaerieMassConfigLoaderSubsystem::GetItemDataTemplate()
{
	ForceItemDataTemplateRegistration();
	return ItemDataMassConfig->GetOrCreateEntityTemplate(*GetWorld());
}

void UFaerieMassConfigLoaderSubsystem::OnItemDataMassConfigLoaded()
{
	ItemDataMassConfigStreamHandle->ForEachLoadedAsset([&](UObject* LoadedObject)
		{
			if (UMassEntityConfigAsset* ConfigAsset = Cast<UMassEntityConfigAsset>(LoadedObject))
			{
				ItemDataMassConfig = ConfigAsset;
				(void)ItemDataMassConfig->GetOrCreateEntityTemplate(*GetWorld());
			}
		});
	ItemDataMassConfigStreamHandle.Reset();
}

void UFaerieMassConfigLoaderSubsystem::ForceItemDataTemplateRegistration()
{
	if (IsValid(ItemDataMassConfig))
	{
		// Config has already been loaded, early out.
		return;
	}

	if (ItemDataMassConfigStreamHandle.IsValid() && ItemDataMassConfigStreamHandle->IsActive())
	{
		// Try flushing the stream handle, which might call OnItemDataMassConfigLoaded.
		ItemDataMassConfigStreamHandle->WaitUntilComplete();
	}

	// If that didn't work, attempt sync loading it directly.
	if (!IsValid(ItemDataMassConfig))
	{
		// Note: Known LoadSync code path; accepted use.
		ItemDataMassConfig = GetDefault<UFaerieItemDataSettings>()->ItemDataMassConfig.LoadSynchronous();
		if (IsValid(ItemDataMassConfig))
		{
			(void)ItemDataMassConfig->GetOrCreateEntityTemplate(*GetWorld());
		}
	}
}