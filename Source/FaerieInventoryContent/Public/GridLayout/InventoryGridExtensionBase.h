// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerDataViewModelBase.h"
#include "NetSupportedObject.h"
#include "FaerieGridEnums.h"
#include "FaerieGridStructs.h"
#include "FaerieItemDataView.h"
#include "FaerieItemProxy.h"
#include "ItemContainerExtensionBase.h"
#include "MassProcessor.h"
#include "InventoryGridExtensionBase.generated.h"

class UFaerieItemStorage;
using FFaerieGridSizeChangedNative = TMulticastDelegate<void(FIntPoint)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFaerieGridSizeChanged, FIntPoint, NewGridSize);

using FFaerieGridStackChangedNative = TMulticastDelegate<void(FFaerieAddress, EFaerieGridEventType)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpatialStackChanged, FFaerieAddress, Address, EFaerieGridEventType, EventType);

namespace Faerie::Extensions
{
	class FCellGrid
	{
	public:
		UE_REWRITE FIntVector2 GetDimensions() const { return Dimensions; }
		bool GetCell(FIntPoint Point) const;

		void Reset(FIntVector2 Size);
		void Resize(FIntVector2 NewSize);

		void MarkCell(const FIntPoint& Point);
		void UnmarkCell(const FIntPoint& Point);

		bool IsEmpty() const;
		bool IsFull() const;
		int32 GetNumCells() const;
		int32 GetNumMarked() const;
		int32 GetNumUnmarked() const;

	protected:
		// Convert a point into a grid index
		int32 Ravel(const FIntPoint& Point) const;

		// Convert a grid index to a point
		FIntPoint Unravel(int32 Index) const;

	private:
		FIntVector2 Dimensions = FIntPoint::ZeroValue;
		TBitArray<> CellBits;
	};
}

class UInventoryGridExtensionBase;

// UObject wrapper around grid data to enable fast array replication. Exposed to Blueprint to allow easier access to
// logic api functions.
UCLASS(BlueprintType)
class UFaerieContainerGridWrapper : public UNetSupportedObject
{
	GENERATED_BODY()

	friend struct FFaerieContainerGridData;

public:
	//~ UObject
	virtual void PostInitProperties() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	//~ UObject

	UPROPERTY()
	TWeakObjectPtr<UFaerieItemStorage> Storage;

	UPROPERTY()
	TObjectPtr<const UInventoryGridExtensionBase> Logic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Config")
	FIntVector2 GridSize = FIntVector2(10, 10);

	UPROPERTY(EditAnywhere, Replicated, Category = "Data")
	FFaerieGridContent GridContent;

	// Locally tracked grid of which cells are occupied.
	Faerie::Extensions::FCellGrid OccupiedCells;
};

USTRUCT(BlueprintType)
struct FFaerieContainerGridReadContext
{
	GENERATED_BODY()

	friend struct FFaerieContainerGridData;
	friend struct FFaerieGridContent;

	const UFaerieItemStorage* GetStorage() const;
	const FFaerieGridContent& GetGrid() const;
	const Faerie::Extensions::FCellGrid& GetOccupiedCells() const;

	FIntVector2 GetGridSize() const;

	TOptional<FFaerieAddress> FindAddress(const FIntPoint& Position) const;

	Faerie::ItemData::FScopeProxy ViewAt_Native(const FIntPoint& Position) const;

	// View the stack on a specified position on the grid.
	FFaerieItemProxy ViewAt(const FIntPoint& Position) const;

	bool IsCellOccupied(const FIntPoint& Position) const;

	bool IsInGrid(FFaerieAddress Address) const;
	FFaerieGridPlacement GetStackPlacementData(FFaerieAddress Address) const;

	bool CanAddAtLocation(const Faerie::TValid<const FFaerieItemProxy&> Proxy, const FIntPoint& Position) const;

	bool IsFull() const { return Data->OccupiedCells.IsFull(); }
	int32 GetEmptyCellCount() const { return Data->OccupiedCells.GetNumUnmarked(); }

protected:
	UPROPERTY()
	TObjectPtr<UFaerieContainerGridWrapper> Data;
};

USTRUCT(NotBlueprintType)
struct FFaerieContainerGridWriteContext final : public FFaerieContainerGridReadContext
{
	GENERATED_BODY()

	friend struct FFaerieContainerGridData;

	UFaerieItemStorage* GetStorage() const;

	void SetGridSize(const FIntPoint& NewGridSize) const;

	bool MoveItem(FFaerieAddress Address, FIntPoint Position) const;
	bool RotateItem(FFaerieAddress Address, EFaerieSpatialItemRotation RotationBy) const;

	UFaerieContainerGridWrapper& Unwrap() const { return *Data; }
};

USTRUCT()
struct FFaerieContainerGridData : public FFaerieItemContainerExtensionBase
{
	GENERATED_BODY()

	//~ FFaerieItemContainerExtensionBase
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual EFaerieExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
	virtual EFaerieExtensionResponse AllowsEdit(TNotNull<const UFaerieItemContainerBase*> Container, const TNotNull<const Faerie::Container::IAddressView*> DataView, FFaerieInventoryTag EditType) const override;
	//~ FFaerieItemContainerExtensionBase

	FFaerieContainerGridReadContext GetReadContext() const;
	FFaerieContainerGridWriteContext GetWriteContext();

	const UInventoryGridExtensionBase* GetGridLogic() const;

	UFaerieContainerGridWrapper* GetData() { return GridWrapper; }
	const UFaerieContainerGridWrapper* GetData() const { return GridWrapper; }

private:
	UPROPERTY()
	TObjectPtr<UFaerieContainerGridWrapper> GridWrapper;

	TSubclassOf<UInventoryGridExtensionBase> GridClass;
};

UCLASS()
class UFaerieContainerGridDataView : public UFaerieContainerDataViewModelBase
{
	GENERATED_BODY()

public:
	//~ UFaerieContainerDataViewModelBase
	virtual void SyncView() override;
	//~ UFaerieContainerDataViewModelBase
};

namespace Faerie::Content
{
	USTRUCT()
	struct FGridDataViewFragment : public Container::FViewModelFragment
	{
		GENERATED_BODY()
	};
}

UCLASS()
class UFaerieContainerGridDataUpdater : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieContainerGridDataUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EventQuery;
	FMassEntityQuery ViewQuery;
};

namespace Faerie::Container
{
	struct FEvent;
}

/**
 * Base class for extensions that map inventory keys to positions on a 2D grid.
 */
UCLASS(Abstract)
class FAERIEINVENTORYCONTENT_API UInventoryGridExtensionBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void InitializeGrid(const FFaerieContainerGridWriteContext& Context) const PURE_VIRTUAL(UInventoryGridExtensionBase::InitializeGrid, ; )

	virtual void PreStackRemove_Client(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const {}
	virtual void PreStackRemove_Server(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack, const FFaerieItemInstance& Item) const {}

	virtual void PostStackAdd(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const {}
	virtual void PostStackChange(const FFaerieContainerGridWriteContext& Context, const FFaerieGridKeyedStack& Stack) const {}

	virtual EFaerieExtensionResponse AllowsAddition(const FFaerieContainerGridReadContext& Context, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const PURE_VIRTUAL(UInventoryGridExtensionBase::AllowsAddition, return EFaerieExtensionResponse::NoExplicitResponse; )
	virtual EFaerieExtensionResponse AllowsEdit(const FFaerieContainerGridReadContext& Context, const TNotNull<const Faerie::Container::IAddressView*> DataView, FFaerieInventoryTag EditType) const PURE_VIRTUAL(UInventoryGridExtensionBase::AllowsEdit, return EFaerieExtensionResponse::NoExplicitResponse; )
	virtual void HandleEvent(const FFaerieContainerGridWriteContext& Context, const Faerie::Container::FEvent& Event) const PURE_VIRTUAL(UInventoryGridExtensionBase::HandleEvent, ; )

	// Publicly accessible actions. Only call on server.
	virtual TOptional<FFaerieAddress> GetKeyAt(const FFaerieContainerGridReadContext& Context, const FIntPoint& Position) const PURE_VIRTUAL(UInventoryGridExtensionBase::GetKeyAt, return NullOpt; )
	virtual bool CanAddAtLocation(const FFaerieContainerGridReadContext& Context, Faerie::TValid<const FFaerieItemProxy&> Proxy, FIntPoint IntPoint) const PURE_VIRTUAL(UInventoryGridExtensionBase::CanAddAtLocation, return false; )
	virtual bool AddItemToGrid(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, const FFaerieItemInstance& Instance) const PURE_VIRTUAL(UInventoryGridExtensionBase::AddItemToGrid, return false; )
	virtual bool MoveItem(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, const FIntPoint& TargetPoint) const PURE_VIRTUAL(UInventoryGridExtensionBase::MoveItem, return false; )
	virtual bool RotateItem(const FFaerieContainerGridWriteContext& Context, FFaerieAddress Address, EFaerieSpatialItemRotation RotationToAdd) const PURE_VIRTUAL(UInventoryGridExtensionBase::RotateItem, return false; )

protected:
	static void BroadcastEvent(FFaerieAddress Address, EFaerieGridEventType EventType);
};

