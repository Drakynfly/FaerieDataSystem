// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Components/ActorComponent.h"
#include "UObject/WeakInterfacePtr.h"
#include "UObject/ObjectKey.h"
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
	  : Proxy(Proxy) {}

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

USTRUCT(Blueprintable)
struct FFaerieVisualMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "EquipmentVisualMetadata")
	FFaerieVisualKey Parent;

	UPROPERTY(EditAnywhere, Category = "EquipmentVisualMetadata")
	FFaerieVisualAttachment Attachment;

	UPROPERTY(EditAnywhere, Category = "EquipmentVisualMetadata")
	FEquipmentVisualizerEvent ChangeCallback;
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

	Faerie::Equipment::FVisualSpawned::RegistrationType& GetOnAnyVisualSpawned() { return OnAnyVisualSpawnedNative; }
	Faerie::Equipment::FVisualDestroyed::RegistrationType& GetOnAnyVisualDestroyed() { return OnAnyVisualDestroyedNative; }

	FGameplayTag GetPreferredTag() const { return PreferredTag; }

	FFaerieVisualSlotElement GetDefaultVisualSlotElement() const;

	// For attachments that want to follow the leader pose, get their leader component.
	USkinnedMeshComponent* GetLeaderComponent() const;

	bool HasVisualForKey(const FFaerieVisualKey& Key) const;

protected:
	void ResetAttachment(const FFaerieVisualKey& Key, const FFaerieVisualAttachment& Attachment);
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

	void CreateVisualImpl(FMassEntityManager& EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, TOptional<const FFaerieItemProxy*> Parent = NullOpt);
	void RemoveVisualImpl(FMassEntityManager& EntityManager, const FFaerieVisualKey& Key);

	template <
		typename TActor
		UE_REQUIRES(TIsDerivedFrom<TActor, AActor>::Value)
	>
	TActor* SpawnVisualActorNative(FFaerieVisualKey Key, const TSubclassOf<TActor>& Class, const FFaerieVisualAttachment& Attachment)
	{
		return Cast<TActor>(SpawnVisualActor(Key, Class, Attachment));
	}

	template <
		typename TComponent
		UE_REQUIRES(TIsDerivedFrom<TComponent, USceneComponent>::Value)
	>
	TComponent* SpawnVisualComponentNative(FFaerieVisualKey Key, const TSubclassOf<TComponent>& Class, const FFaerieVisualAttachment& Attachment)
	{
		return Cast<TComponent>(SpawnVisualComponent(Key, Class, Attachment));
	}

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (AutoCreateRefTerm = "Attachment", DeterminesOutputType = "Class"))
	AActor* SpawnVisualActor(FFaerieVisualKey Key, const TSubclassOf<AActor>& Class, const FFaerieVisualAttachment& Attachment);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (AutoCreateRefTerm = "Attachment", DeterminesOutputType = "Class"))
	USceneComponent* SpawnVisualComponent(FFaerieVisualKey Key, const TSubclassOf<USceneComponent>& Class, const FFaerieVisualAttachment& Attachment);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	bool DestroyVisual(UObject* Visual, bool ClearMetadata = false);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	bool DestroyVisualByKey(FFaerieVisualKey Key, bool ClearMetadata = false);

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

	// @todo not used
	//UFUNCTION(/* Dynamic Callback */)
	//void OnVisualComponentDestroyed(USceneComponent* DestroyedComponent);

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FFaerieEquipmentVisualSpawned OnAnyVisualSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FFaerieEquipmentVisualDestroyed OnAnyVisualDestroyed;

protected:
	// The MeshPurpose preferred by this Visualizer.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (Categories = "MeshPurpose"))
	FGameplayTag PreferredTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	FComponentReference LeaderPoseComponent;

	UPROPERTY()
	TMap<FFaerieVisualKey, TObjectPtr<AActor>> SpawnedActors;

	UPROPERTY()
	TMap<FFaerieVisualKey, TObjectPtr<USceneComponent>> SpawnedComponents;
	TCompactMap<FObjectKey, FFaerieVisualKey> ReverseMap;

	UPROPERTY()
	TMap<FFaerieVisualKey, FFaerieVisualMetadata> KeyedMetadata;

private:
	Faerie::Equipment::FVisualSpawned OnAnyVisualSpawnedNative;
	Faerie::Equipment::FVisualDestroyed OnAnyVisualDestroyedNative;

	struct FPendingAttachment
	{
		FFaerieVisualKey Key;
		FFaerieVisualAttachment Attachment;
		bool ParentReady = false;
		bool ChildReady = false;
	};
	TArray<FPendingAttachment> Pending;
};