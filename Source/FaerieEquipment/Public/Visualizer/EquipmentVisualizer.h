// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "EquipmentVisualizer.generated.h"

class UFaerieItemContainerBase;
class AFaerieProxyActorBase;
class UFaerieItemMeshComponent;
struct FFaerieVisualSlotElement;

USTRUCT(BlueprintType)
struct FFaerieVisualKey
{
	GENERATED_BODY()

	FFaerieVisualKey() {}
	FFaerieVisualKey(const FFaerieItemProxy& Proxy)
	  : Proxy(Proxy)
	{
		check(Proxy.IsSafeToPersist())
	}

private:
	UPROPERTY()
	FFaerieItemProxy Proxy;

public:
	UE_REWRITE const FFaerieItemProxy& GetProxy() const { return Proxy; }

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieVisualKey& Other) const { return Proxy == Other.Proxy; }

	[[nodiscard]] UE_REWRITE friend uint32 GetTypeHash(const FFaerieVisualKey& VisualKey)
	{
		return GetTypeHash(VisualKey.Proxy);
	}
};

USTRUCT(Blueprintable)
struct FFaerieVisualAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentVisualAttachment")
	TWeakObjectPtr<USceneComponent> Parent;

	// The attachment point on the parent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentVisualAttachment")
	FName ParentSocket;

	// The attachment point on the child
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentVisualAttachment")
	FName ChildSocket;

	// Is this visual currently hidden (will not render)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentVisualAttachment")
	bool Hidden = false;

	FAttachmentTransformRules TransformRules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FEquipmentVisualizerCallback, FFaerieVisualKey, Key, UObject*, Visual);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentVisualizerEvent, const FFaerieVisualKey&, Key, UObject*, Visual);

USTRUCT()
struct FFaerieVisualizationEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UObject> Visual;

	UPROPERTY()
	FFaerieVisualKey Parent;

	UPROPERTY()
	FFaerieVisualAttachment Attachment;

	// Broadcast when a new visual is fully setup.
	UPROPERTY()
	FEquipmentVisualizerEvent ChangeCallback;

	// Are we ready to be attached to our parent?
	bool SelfReady = false;

	// Is our parent ready for us to attach to it?
	bool ParentReady = false;
};

namespace Faerie::Equipment
{
	using FVisualSpawned = TMulticastDelegate<void(const FFaerieVisualKey&, TNotNull<UObject*>)>;
	using FVisualDestroyed = TMulticastDelegate<void(const FFaerieVisualKey&)>;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFaerieEquipmentVisualSpawned, const FFaerieVisualKey&, Key, UObject*, Visual);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFaerieEquipmentVisualDestroyed, const FFaerieVisualKey&, Key);

// @todo move this to ItemMesh module, rename to UFaerieVisualizationComponent
/**
 * An actor component that spawns visuals for items on the actor
 */
UCLASS(ClassGroup = ("Faerie"), meta=(BlueprintSpawnableComponent))
class FAERIEEQUIPMENT_API UEquipmentVisualizer : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquipmentVisualizer();

	//~ UActorComponent
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ UActorComponent

	FGameplayTag GetPreferredTag() const { return PreferredTag; }

	FFaerieVisualSlotElement GetDefaultVisualSlotElement() const;

	// For attachments that want to follow the leader pose, get their leader component.
	USkinnedMeshComponent* GetLeaderComponent() const;

	bool HasVisualForKey(const FFaerieVisualKey& Key) const;

protected:
	void ResetAttachment(const FFaerieVisualizationEntry& Entry);
	void PostVisualComponentLoaded(const FFaerieVisualKey& Key, TNotNull<UFaerieItemMeshComponent*> Component);
	void PostVisualActionLoaded(const FFaerieVisualKey& Key, TNotNull<AFaerieProxyActorBase*> Actor);

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (DeterminesOutputType = "Class"))
	UObject* GetSpawnedVisualByClass(TSubclassOf<UObject> Class, FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (DeterminesOutputType = "Class"))
	AActor* GetSpawnedActorByClass(TSubclassOf<AActor> Class, FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (DeterminesOutputType = "Class"))
	USceneComponent* GetSpawnedComponentByClass(TSubclassOf<USceneComponent> Class, FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	UObject* GetSpawnedVisualByKey(const FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	AActor* GetSpawnedActorByKey(const FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	USceneComponent* GetSpawnedComponentByKey(const FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	TArray<AActor*> GetSpawnedActors() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	TArray<USceneComponent*> GetSpawnedComponents() const;

	void FlushInvalid(FMassEntityManager& EntityManager);
	void RefreshVisual(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy);

private:
	void CreateVisualImpl(FMassEntityManager& EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, TOptional<const FFaerieItemProxy*> Parent = NullOpt);
	void RemoveVisualImpl(FMassEntityManager& EntityManager, const FFaerieVisualKey& Key);

	AActor* SpawnVisualActor(FFaerieVisualKey Key, const TSubclassOf<AActor>& Class);

	USceneComponent* SpawnVisualComponent(FFaerieVisualKey Key, const TSubclassOf<USceneComponent>& Class);

	template <
		typename TActor
		UE_REQUIRES(TIsDerivedFrom<TActor, AActor>::Value)
	>
	TActor* SpawnVisualActorNative(FFaerieVisualKey Key, const TSubclassOf<TActor>& Class)
	{
		return Cast<TActor>(SpawnVisualActor(Key, Class));
	}

	template <
		typename TComponent
		UE_REQUIRES(TIsDerivedFrom<TComponent, USceneComponent>::Value)
	>
	TComponent* SpawnVisualComponentNative(FFaerieVisualKey Key, const TSubclassOf<TComponent>& Class)
	{
		return Cast<TComponent>(SpawnVisualComponent(Key, Class));
	}

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void DestroyVisual(UObject* Visual, bool ClearMetadata = false);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void DestroyVisualByKey(FFaerieVisualKey Key, bool ClearMetadata = false);

	// Determine how a visual should attach
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	FFaerieVisualAttachment FindAttachment(const FFaerieItemProxy& Proxy) const;

	static TOptional<FFaerieVisualSlotElement> FindVisualSlotData(const FMassEntityManager& EntityManager, TNotNull<const UFaerieItemContainerBase*> Container);

	USceneComponent* GetComponentToAttachTo(const FMassEntityManager& EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container, const FName& ComponentTag, bool& WantsToWaitForParentVisual) const;
	static FFaerieVisualAttachment BuildAttachmentData(USceneComponent* AttachTo, const FFaerieVisualSlotElement& SlotData);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void ResetAttachment(FFaerieVisualKey Key);

	// Moves where a visual is attached, but doesn't make the change permanent. (It can be undone with ResetAttachment)
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void MoveAttachment(FFaerieVisualKey Key, const FFaerieVisualAttachment& Attachment);

	// Moves where a visual is attached, and makes the change permanent. (It cannot be undone with ResetAttachment)
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void UpdateAttachment(FFaerieVisualKey Key, const FFaerieVisualAttachment& Attachment);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void AwaitOrReceiveUpdate(FFaerieVisualKey Key, FEquipmentVisualizerCallback Callback);

	UFUNCTION(BlueprintPure, Category = "Faerie|EquipmentVisualizer")
	static FFaerieVisualKey MakeVisualKey(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|EquipmentVisualizer")
	void DumpDebugLog() const;

	// Debug function to test if attachments get out of sync.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void RefreshAllAttachments();

protected:
	UFUNCTION(/* Dynamic Callback */)
	void OnVisualActorDestroyed(AActor* DestroyedActor);

protected:
	// The MeshPurpose preferred by this Visualizer.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (Categories = "MeshPurpose"))
	FGameplayTag PreferredTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FComponentReference LeaderPoseComponent;

	UPROPERTY()
	TMap<FFaerieVisualKey, FFaerieVisualizationEntry> VisualData;
};