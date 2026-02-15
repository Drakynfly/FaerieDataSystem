// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "CapacityStructs.h"
#include "InventoryCapacityExtension.generated.h"

UENUM(BlueprintType, Flags, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECapacityChecks : uint8
{
    None    = 0 UMETA(Hidden),

    // Check that the item does not exceed physical dimensions
    Bounds  = 1 << 0,

    // Check that the item does not exceed a maximum weight
    Weight  = 1 << 1,

    // Check that the item does not exceed a maximum volume
    Volume  = 1 << 2,

    // Require items to have a CapacityToken
    Token   = 1 << 3
};
ENUM_CLASS_FLAGS(ECapacityChecks)

USTRUCT(BlueprintType)
struct FCapacityExtensionConfig
{
    GENERATED_BODY()

    /** Which capacity checks are performed when determining if an entry can "fit" in the inventory. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capacity Config", meta = (Bitmask, BitmaskEnum = "/Script/FaerieInventoryContent.ECapacityChecks"))
    int32 Checks = 0;

    /** Size in centimeters of the maximum bounding box for contained items. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capacity Config", meta = (Units = cm))
    FIntVector Bounds = FIntVector(0);

    /** How much items can exceed the bounds, but still be allowed to be contained. Useful for soft containers */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capacity Config")
    float BoundsFudgeFactor = 1.1f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capacity Config")
    FWeightEditor MaxWeight = 0;

    /** If enabled, MaxVolume will be set to Bounds.X*Bounds.Y*Bounds.Z. Disable to manually edit MaxVolume */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capacity Config")
    bool DeriveVolumeFromBounds = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Capacity Config", meta = (EditCondition = "!DeriveVolumeFromBounds"))
    int64 MaxVolume = 0;

    bool HasCheck(const ECapacityChecks Check) const
    {
        return EnumHasAnyFlags(static_cast<ECapacityChecks>(Checks), Check);
    }
};

/**
 * The state of the capacity that is replicated to all clients.
 */
USTRUCT(BlueprintType)
struct FCapacityExtensionState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Weight", meta = (Units = g))
    int32 CurrentWeight = 0;

    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Volume")
    int64 CurrentVolume = 0;

    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Over Max Status", meta = (AllowPrivateAccess = true))
    bool OverMaxWeight = false;

    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Over Max Status", meta = (AllowPrivateAccess = true))
    bool OverMaxVolume = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryCapacityEvent);

/**
 * This class uses the Capacity Tokens on items to maintain an aggregated capacity of all items across each
 * registered container. This allows for keeping track of a summed weight and volume of inventory content.
 * Additionally, in the Config, Checks can be enabled, to disable adding more items to the container once limits
 * are reached.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryCapacityExtension : public UItemContainerExtensionBase
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

protected:
    //~ UItemContainerExtensionBase
    virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
    virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
    virtual EEventExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, TConstArrayView<FFaerieItemStackView> Views, FFaerieExtensionAllowsAdditionArgs Args) const override;
    virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) override;
    //~ UItemContainerExtensionBase

private:
    static FWeightAndVolume GetEntryWeightAndVolume(TNotNull<const UFaerieItemContainerBase*> Container, const FEntryKey Key);

    void UpdateCacheForEntry(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key);

    void CheckCapacityLimit();

    bool CanContainToken(const class UFaerieCapacityToken* Token, const int32 Stack) const;

    void AddWeightAndVolume(FWeightAndVolume Value);

    void HandleStateChanged();

public:
    FSimpleMulticastDelegate::RegistrationType& GetOnStateChanged() { return OnStateChangedNative; }
    FSimpleMulticastDelegate::RegistrationType& GetOnConfigurationChanged() { return OnConfigurationChangedNative; }

    // Tests if the capacity of an stack can fit in this container.
    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    bool CanContain(FFaerieItemStackView Stack) const;

    // Tests if the capacity of multiple stacks can fit in this container at once.
    //UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    bool CanContain_Multi(TConstArrayView<FFaerieItemStackView> Stacks) const;

    // Tests if the capacity of an item can fit in this container.
    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    bool CanContainProxy(FFaerieItemProxy Proxy) const;

    // Get the configuration struct.
    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    const FCapacityExtensionConfig& GetCapacityConfig() const { return Config; }

    // Get the current state struct.
    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    FCapacityExtensionState GetCurrentState() const { return State; }

    // Get the current amount filled.
    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    FWeightAndVolume GetCurrentCapacity() const;

    // Get the maximum amount that this manager can hold.
    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    FWeightAndVolume GetMaxCapacity() const;

    UFUNCTION(BlueprintCallable, Category = "Faerie|InventoryCapacity")
    void SetConfiguration(const FCapacityExtensionConfig& NewConfig);

    UFUNCTION(BlueprintCallable, Category = "Faerie|InventoryCapacity")
    void SetBounds(const FIntVector NewBounds);

    UFUNCTION(BlueprintCallable, Category = "Faerie|InventoryCapacity")
    void SetMaxCapacity(const FWeightAndVolume NewMax);

    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    float GetPercentageFullForWeightAndVolume(const FWeightAndVolume& WeightAndVolume) const;

    // Get our current percentage "fullness"
    UFUNCTION(BlueprintPure, Category = "Faerie|InventoryCapacity")
    float GetPercentageFull() const;

protected:
    UFUNCTION(/* Replication */)
    virtual void OnRep_Config();

    UFUNCTION(/* Replication */)
    virtual void OnRep_State();

    // Broadcast whenever the state changes.
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FInventoryCapacityEvent OnStateChanged;

    // Broadcast whenever the config changes.
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FInventoryCapacityEvent OnConfigurationChanged;

    UPROPERTY(ReplicatedUsing = "OnRep_Config", EditAnywhere, Category = "Capacity", meta = (ShowOnlyInnerProperties))
    FCapacityExtensionConfig Config;

    UPROPERTY(ReplicatedUsing = "OnRep_State", VisibleInstanceOnly, Category = "Capacity")
    FCapacityExtensionState State;

    // Cache of all entries to maintain serverside integrity.
    // @todo actually use this to validate State
    TMap<TWeakObjectPtr<const UFaerieItemContainerBase>, TMap<FEntryKey, FWeightAndVolume>> ServerCapacityCache;

private:
    FSimpleMulticastDelegate OnStateChangedNative;
    FSimpleMulticastDelegate OnConfigurationChangedNative;
};