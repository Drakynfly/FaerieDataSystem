// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Components/ActorComponent.h"
#include "UObject/WeakInterfacePtr.h"
#include "GameplayTagContainer.h"

#include "EquipmentVisualizer.generated.h"

USTRUCT(BlueprintType)
struct FFaerieVisualKey
{
	GENERATED_BODY()

	FFaerieItemProxy Proxy;

	bool IsValid() const
	{
		return Proxy.IsValid();
	}

	friend bool operator==(const FFaerieVisualKey& Lhs, const FFaerieVisualKey& Rhs) { return Lhs.Proxy == Rhs.Proxy; }
	friend bool operator!=(const FFaerieVisualKey& Lhs, const FFaerieVisualKey& Rhs) { return !(Lhs == Rhs); }

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieVisualKey& VisualKey)
	{
		return GetTypeHash(VisualKey.Proxy.GetObject());
	}
};

USTRUCT(Blueprintable)
struct FEquipmentVisualAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentVisualAttachment")
	TWeakObjectPtr<USceneComponent> Parent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EquipmentVisualAttachment")
	FName Socket;

	FAttachmentTransformRules TransformRules = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FEquipmentVisualizerCallback, FFaerieVisualKey, Key, UObject*, Visual);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipmentVisualizerEvent, FFaerieVisualKey, Key, UObject*, Visual);

USTRUCT(Blueprintable)
struct FEquipmentVisualMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "EquipmentVisualMetadata")
	FEquipmentVisualAttachment Attachment;

	UPROPERTY(EditAnywhere, Category = "EquipmentVisualMetadata")
	FEquipmentVisualizerEvent ChangeCallback;
};

namespace Faerie::Equipment
{
	using FVisualSpawned = TMulticastDelegate<void(FFaerieVisualKey, UObject*)>;
	using FVisualDestroyed = TMulticastDelegate<void(FFaerieVisualKey)>;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFaerieEquipmentVisualSpawned, FFaerieVisualKey, Key, UObject*, Visual);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFaerieEquipmentVisualDestroyed, FFaerieVisualKey, Key);

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

	// For attachments that want to follow the leader pose, get their leader component.
	USkinnedMeshComponent* GetLeaderComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (DeterminesOutputType = "Class"))
	UObject* GetSpawnedVisualByClass(TSubclassOf<UObject> Class, FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (DeterminesOutputType = "Class"))
	AActor* GetSpawnedActorByClass(TSubclassOf<AActor> Class, FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (DeterminesOutputType = "Class"))
	USceneComponent* GetSpawnedComponentByClass(TSubclassOf<USceneComponent> Class, FFaerieVisualKey& Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	UObject* GetSpawnedVisualByKey(FFaerieVisualKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	AActor* GetSpawnedActorByKey(FFaerieVisualKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	USceneComponent* GetSpawnedComponentByKey(FFaerieVisualKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	TArray<AActor*> GetSpawnedActors() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	TArray<USceneComponent*> GetSpawnedComponents() const;

	template <
		typename TActor
		UE_REQUIRES(TIsDerivedFrom<TActor, AActor>::Value)
	>
	TActor* SpawnVisualActorNative(FFaerieVisualKey Key, const TSubclassOf<TActor>& Class, const FEquipmentVisualAttachment& Attachment)
	{
		return Cast<TActor>(SpawnVisualActor(Key, Class, Attachment));
	}

	template <
		typename TComponent
		UE_REQUIRES(TIsDerivedFrom<TComponent, USceneComponent>::Value)
	>
	TComponent* SpawnVisualComponentNative(FFaerieVisualKey Key, const TSubclassOf<TComponent>& Class, const FEquipmentVisualAttachment& Attachment)
	{
		return Cast<TComponent>(SpawnVisualComponent(Key, Class, Attachment));
	}

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (AutoCreateRefTerm = "Attachment", DeterminesOutputType = "Class"))
	AActor* SpawnVisualActor(FFaerieVisualKey Key, const TSubclassOf<AActor>& Class, const FEquipmentVisualAttachment& Attachment);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer", meta = (AutoCreateRefTerm = "Attachment", DeterminesOutputType = "Class"))
	USceneComponent* SpawnVisualComponent(FFaerieVisualKey Key, const TSubclassOf<USceneComponent>& Class, const FEquipmentVisualAttachment& Attachment);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	bool DestroyVisual(UObject* Visual, bool ClearMetadata = false);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	bool DestroyVisualByKey(FFaerieVisualKey Key, bool ClearMetadata = false);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void ResetAttachment(FFaerieVisualKey Key);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentVisualizer")
	void AwaitOrReceiveUpdate(FFaerieVisualKey Key, FEquipmentVisualizerCallback Callback);

	UFUNCTION(BlueprintPure)
	static FFaerieVisualKey MakeVisualKeyFromProxy(const TScriptInterface<IFaerieItemDataProxy>& Proxy);

protected:
	UFUNCTION(/* Dynamic Callback */)
	virtual void OnVisualActorDestroyed(AActor* DestroyedActor);

	// @todo not used
	//UFUNCTION(/* Dynamic Callback */)
	//virtual void OnVisualComponentDestroyed(USceneComponent* DestroyedComponent);

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

	UPROPERTY()
	TMap<TObjectPtr<UObject>, FFaerieVisualKey> ReverseMap;

	UPROPERTY()
	TMap<FFaerieVisualKey, FEquipmentVisualMetadata> KeyedMetadata;

private:
	Faerie::Equipment::FVisualSpawned OnAnyVisualSpawnedNative;
	Faerie::Equipment::FVisualDestroyed OnAnyVisualDestroyedNative;
};