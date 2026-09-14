// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "CapacityStructs.h"
#include "FaerieContainerDataViewModelBase.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemProxy.h"
#include "MVVMViewModelBase.h"
#include "MassProcessor.h"
#include "InventoryCapacityExtension.generated.h"

#define FAE_API FAERIEINVENTORYCONTENT_API

namespace Faerie::Container
{
    struct FEvent;
}

class UFaerieItemContainerCapacityView;

UENUM(BlueprintType, Flags, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EFaerieCapacityExtensionChecks : uint8
{
    None    = 0 UMETA(Hidden),

    // Check that the item does not exceed physical dimensions
    Bounds  = 1 << 0,

    // Check that the item does not exceed a maximum weight
    Weight  = 1 << 1,

    // Check that the item does not exceed a maximum volume
    Volume  = 1 << 2,

    // Require items to have a Capacity Fragment
    Fragment   = 1 << 3
};
ENUM_CLASS_FLAGS(EFaerieCapacityExtensionChecks)

USTRUCT(BlueprintType)
struct FFaerieCapacityExtensionConfig
{
    GENERATED_BODY()

    /** Which capacity checks are performed when determining if an entry can "fit" in the inventory. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CapacityExtensionConfig", meta = (Bitmask, BitmaskEnum = "/Script/FaerieInventoryContent.EFaerieCapacityExtensionChecks"))
    int32 Checks = 0;

    /** Size in centimeters of the maximum bounding box for contained items. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CapacityExtensionConfig", meta = (Units = cm))
    FIntVector Bounds = FIntVector(0);

    /** How much items can exceed the bounds, but still be allowed to be contained. Useful for soft containers */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CapacityExtensionConfig")
    float BoundsFudgeFactor = 1.1f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CapacityExtensionConfig")
    FFaerieWeightEditor MaxWeight = 0;

    /** If enabled, MaxVolume will be set to Bounds.X*Bounds.Y*Bounds.Z. Disable to manually edit MaxVolume */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CapacityExtensionConfig")
    bool DeriveVolumeFromBounds = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CapacityExtensionConfig", meta = (EditCondition = "!DeriveVolumeFromBounds"))
    int64 MaxVolume = 0;

    bool HasCheck(const EFaerieCapacityExtensionChecks Check) const
    {
        return EnumHasAnyFlags(static_cast<EFaerieCapacityExtensionChecks>(Checks), Check);
    }
};

/**
 * The state of the capacity that is replicated to all clients.
 */
USTRUCT(BlueprintType)
struct FFaerieCapacityExtensionState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "CapacityExtensionState", meta = (Units = g))
    int32 CurrentWeight = 0;

    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "CapacityExtensionState")
    int64 CurrentVolume = 0;
};

USTRUCT()
struct FFaerieItemContainerCapacityData : public FFaerieItemContainerExtensionBase
{
    GENERATED_BODY()

    friend class UFaerieItemContainerCapacityUpdater;

    //~ FFaerieItemContainerExtensionBase
    virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
    virtual EFaerieExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
    //~ FFaerieItemContainerExtensionBase

    const FFaerieCapacityExtensionConfig& GetConfig() const { return Config; }
    const FFaerieCapacityExtensionState& GetState() const { return State; }

private:
    bool HandleEvent(const FMassEntityManager& EntityManager, const Faerie::Container::FEvent& Event);

    // Tests if the capacity of a stack can fit in this container.
    bool CanContain(const FMassEntityManager& EntityManager, TNotNull<const UFaerieItemContainerBase*> Container, Faerie::TValid<const FFaerieItemProxy&> Proxy) const;

    // Tests if the capacity of multiple stacks can fit in this container at once.
    bool CanContain_Multi(const FMassEntityManager& EntityManager, TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies) const;

    bool UpdateCacheForEntry(const FMassEntityManager& EntityManager, TNotNull<const UFaerieItemContainerBase*> Container, FFaerieEntryKey Key);
    bool RemoveCacheForEntry(TNotNull<const UFaerieItemContainerBase*> Container, FFaerieEntryKey Key);

    void AddWeightAndVolume(FFaerieWeightAndVolume Value);

protected:
    UPROPERTY(EditAnywhere, Category = "Capacity", meta = (ShowOnlyInnerProperties))
    FFaerieCapacityExtensionConfig Config;

    UPROPERTY(VisibleInstanceOnly, Category = "Capacity", meta = (ShowOnlyInnerProperties))
    FFaerieCapacityExtensionState State;

    TMap<FFaerieEntryKey, FFaerieWeightAndVolume> EntryCache;

____FAERIE_CONTAINER_DATA_DECL(FFaerieItemContainerCapacityData)
};

UCLASS()
class UFaerieItemContainerCapacityView : public UFaerieContainerDataViewModelBase
{
    GENERATED_BODY()

public:
    //~ UFaerieContainerDataViewModelBase
    virtual void SyncView() override;
    //~ UFaerieContainerDataViewModelBase

    UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainerCapacityView")
    void SetConfiguration(const FFaerieCapacityExtensionConfig& NewConfig);

    UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainerCapacityView")
    void SetBounds(const FIntVector NewBounds);

    UFUNCTION(BlueprintCallable, Category = "Faerie|ItemContainerCapacityView")
    void SetMaxCapacity(const FFaerieWeightAndVolume NewMax);

protected:
    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ItemContainerCapacityView", meta = (ShowOnlyInnerProperties))
    FFaerieCapacityExtensionConfig Config;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ItemContainerCapacityView", meta = (ShowOnlyInnerProperties))
    FFaerieCapacityExtensionState State;
};

namespace Faerie::Content
{
    USTRUCT()
    struct FCapacityViewFragment : public Container::FViewModelFragment
    {
        GENERATED_BODY()
    };
}

UCLASS()
class UFaerieItemContainerCapacityUpdater : public UMassProcessor
{
    GENERATED_BODY()

public:
    UFaerieItemContainerCapacityUpdater();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    FMassEntityQuery EventQuery;
    FMassEntityQuery ViewQuery;
};

#undef FAE_API