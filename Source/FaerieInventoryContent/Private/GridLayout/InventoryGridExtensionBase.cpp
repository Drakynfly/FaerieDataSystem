// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"

#include "GridLayout/InventoryGridExtensionBase.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"
#include "FaerieItemStorageIterators.h"
#include "MassExecutionContext.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryGridExtensionBase)

namespace Faerie::Extensions
{
	bool FCellGrid::GetCell(const FIntPoint Point) const
	{
		const int32 Index = Ravel(Point);
		if (!CellBits.IsValidIndex(Index))
		{
			// If cell doesn't exist, it cannot be occupied
			return false;
		}
		return CellBits[Index];
	}

	void FCellGrid::Reset(const FIntVector2 Size)
	{
		Dimensions = Size;
		CellBits.Init(false, Size.X * Size.Y);
	}

	void FCellGrid::Resize(const FIntVector2 NewSize)
	{
		const FIntVector2 OldSize = Dimensions;
		TBitArray<> OldBits = CellBits;

        Reset(NewSize);

		// Copy over existing data that's still in bounds
		for (int32 y = 0; y < FMath::Min(OldSize.Y, NewSize.Y); y++)
		{
			for (int32 x = 0; x < FMath::Min(OldSize.X, NewSize.X); x++)
			{
				const int32 OldIndex = x + y * OldSize.X;
				const int32 NewIndex = x + y * NewSize.X;
				CellBits[NewIndex] = OldBits[OldIndex];
			}
		}
	}

	void FCellGrid::MarkCell(const FIntPoint& Point)
	{
		const int32 Index = Ravel(Point);
		if (!CellBits.IsValidIndex(Index))
		{
			// If cell doesn't exist, expand to fit.
			CellBits.SetNum(Index, false);
		}
		CellBits[Index] = true;
	}

	void FCellGrid::UnmarkCell(const FIntPoint& Point)
	{
		const int32 Index = Ravel(Point);
		if (!CellBits.IsValidIndex(Index))
		{
			// If cell doesn't exist, no need to unmark it.
			return;
		}
		CellBits[Index] = false;
	}

	bool FCellGrid::IsEmpty() const
	{
		return !CellBits.Contains(true);
	}

	bool FCellGrid::IsFull() const
	{
		return !CellBits.Contains(false);
	}

	int32 FCellGrid::GetNumCells() const
	{
		return Dimensions.X * Dimensions.Y;
	}

	int32 FCellGrid::GetNumMarked() const
	{
		return CellBits.CountSetBits();
	}

	int32 FCellGrid::GetNumUnmarked() const
	{
		return GetNumCells() - GetNumMarked();
	}

	int32 FCellGrid::Ravel(const FIntPoint& Point) const
	{
		return Point.Y * Dimensions.X + Point.X;
	}

	FIntPoint FCellGrid::Unravel(const int32 Index) const
	{
		const int32 X = Index % Dimensions.X;
		const int32 Y = Index / Dimensions.X;
		return FIntPoint{ X, Y };
	}
}

using namespace Faerie;

void UFaerieContainerGridWrapper::PostInitProperties()
{
	Super::PostInitProperties();
	GridContent.ChangeListener = this;
}

void UFaerieContainerGridWrapper::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, GridContent, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, GridSize, SharedParams)
}

void FFaerieContainerGridData::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	UFaerieItemStorage* Storage = const_cast<UFaerieItemStorage*>(CastChecked<UFaerieItemStorage>(Container));
	GridWrapper = NewObject<UFaerieContainerGridWrapper>();
	GridWrapper->Storage = Storage;
	GridWrapper->Logic = GetGridLogic();

	return GetGridLogic()->InitializeGrid(GetWriteContext());
}

EFaerieExtensionResponse FFaerieContainerGridData::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
	const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	check(Container == GridWrapper->Storage);
	return GetGridLogic()->AllowsAddition(GetReadContext(), Proxies, Args);
}

EFaerieExtensionResponse FFaerieContainerGridData::AllowsEdit(const TNotNull<const UFaerieItemContainerBase*> Container,
	const TNotNull<const Container::IAddressView*> DataView, const FFaerieInventoryTag EditType) const
{
	check(Container == GridWrapper->Storage);
	return GetGridLogic()->AllowsEdit(GetReadContext(), DataView, EditType);
}

FFaerieContainerGridReadContext FFaerieContainerGridData::GetReadContext() const
{
	FFaerieContainerGridReadContext Context;
	Context.Data = GridWrapper;
	return Context;
}

FFaerieContainerGridWriteContext FFaerieContainerGridData::GetWriteContext()
{
	FFaerieContainerGridWriteContext Context;
	Context.Data = GridWrapper;
	return Context;
}

const UInventoryGridExtensionBase* FFaerieContainerGridData::GetGridLogic() const
{
	return GridClass.GetDefaultObject();
}

void UFaerieContainerGridDataView::SyncView()
{
	// @Todo implement
}

UFaerieContainerGridDataUpdater::UFaerieContainerGridDataUpdater()
  : EventQuery(*this), ViewQuery(*this)
{
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::AllNetModes);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieContainerGridDataUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<Container::FEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	ViewQuery.AddRequirement<Content::FGridDataViewFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieContainerGridDataUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Track the containers we updated, so we can notify any active views of them.
	TSet<UObject*> ContainersUpdated;

	EventQuery.ForEachEntityChunk(Context, [this, &ContainersUpdated](const FMassExecutionContext& InContext)
		{
			const TConstArrayView<Container::FEvent> Events = InContext.GetFragmentView<Container::FEvent>();
			for (const Container::FEvent& Event : Events)
			{
				UFaerieItemStorage* Storage = Cast<UFaerieItemStorage>(Event.Container.Get());
				if (!IsValid(Storage))
				{
					continue;
				}

				Storage->WriteContainerData(FFaerieContainerGridData::StaticStruct(), [&InContext, &Event, Storage](const FStructView Element)
				{
					auto& GridData = Element.Get<FFaerieContainerGridData>();
					GridData.GetGridLogic()->HandleEvent(GridData.GetWriteContext(), Event);
				}, false);

				ContainersUpdated.Add(Storage);
			}
		});

	ViewQuery.ForEachEntityChunk(Context, [&ContainersUpdated](const FMassExecutionContext& InContext)
		{
			const TConstArrayView<Container::FViewModelFragment> Views = InContext.GetFragmentView<Container::FViewModelFragment>(Content::FGridDataViewFragment::StaticStruct());
			for (const Container::FViewModelFragment& ViewFragment : Views)
			{
				UFaerieContainerDataViewModelBase* View = Cast<UFaerieContainerDataViewModelBase>(ViewFragment.ViewObject.Get());
				if (!IsValid(View))
				{
					continue;
				}

				if (ContainersUpdated.Contains(View->GetContainerObject()))
				{
					View->SyncView();
				}
			}
		});
}

void UInventoryGridExtensionBase::BroadcastEvent(const FFaerieAddress Address, const EFaerieGridEventType EventType)
{
	// @Todo broadcast event for views???
}

const UFaerieItemStorage* FFaerieContainerGridReadContext::GetStorage() const
{
	return Data->Storage.Get();
}

const FFaerieGridContent& FFaerieContainerGridReadContext::GetGrid() const
{
	return Data->GridContent;
}

const Extensions::FCellGrid& FFaerieContainerGridReadContext::GetOccupiedCells() const
{
	return Data->OccupiedCells;
}

FIntVector2 FFaerieContainerGridReadContext::GetGridSize() const
{
	return Data->GridSize;
}

TOptional<FFaerieAddress> FFaerieContainerGridReadContext::FindAddress(const FIntPoint& Position) const
{
	return Data->Logic->GetKeyAt(*this, Position);
}

ItemData::FScopeProxy FFaerieContainerGridReadContext::ViewAt_Native(const FIntPoint& Position) const
{
	if (const TOptional<FFaerieAddress> Address = Data->Logic->GetKeyAt(*this, Position);
		Address.IsSet())
	{
		return Data->Storage->ViewAddress(Address.GetValue());
	}
	return nullptr;
}

FFaerieItemProxy FFaerieContainerGridReadContext::ViewAt(const FIntPoint& Position) const
{
	if (const TOptional<FFaerieAddress> Address = Data->Logic->GetKeyAt(*this, Position);
		Address.IsSet())
	{
		return Data->Storage->Proxy(Address.GetValue());
	}
	return FFaerieItemProxy();
}

bool FFaerieContainerGridReadContext::IsCellOccupied(const FIntPoint& Position) const
{
	return Data->OccupiedCells.GetCell(Position);
}

bool FFaerieContainerGridReadContext::IsInGrid(const FFaerieAddress Address) const
{
	return Data->GridContent.Contains(Address);
}

FFaerieGridPlacement FFaerieContainerGridReadContext::GetStackPlacementData(const FFaerieAddress Address) const
{
	if (const FFaerieGridKeyedStack* KeyedStack = Data->GridContent.BSOA::Find(Address))
	{
		return KeyedStack->Value;
	}

	return FFaerieGridPlacement();
}

bool FFaerieContainerGridReadContext::CanAddAtLocation(const TValid<const FFaerieItemProxy&> Proxy, const FIntPoint& Position) const
{
	return Data->Logic->CanAddAtLocation(*this, Proxy, Position);
}

UFaerieItemStorage* FFaerieContainerGridWriteContext::GetStorage() const
{
	return Data->Storage.Get();
}

void FFaerieContainerGridWriteContext::SetGridSize(const FIntPoint& NewGridSize) const
{
	if (Data->GridSize != NewGridSize)
	{
		// Resize to new dimensions
		Data->GridSize = NewGridSize;
		Data->OccupiedCells.Resize(Data->GridSize);

		MARK_PROPERTY_DIRTY_FROM_NAME(UFaerieContainerGridWrapper, GridSize, Data);

		// OnReps must be called manually on the server in c++
		//OnRep_GridSize();
		// @todo alert view???
	}
}

bool FFaerieContainerGridWriteContext::MoveItem(const FFaerieAddress Address, const FIntPoint Position) const
{
	return Data->Logic->MoveItem(*this, Address, Position);
}

bool FFaerieContainerGridWriteContext::RotateItem(const FFaerieAddress Address, const EFaerieSpatialItemRotation RotationBy) const
{
	return Data->Logic->RotateItem(*this, Address, RotationBy);
}
