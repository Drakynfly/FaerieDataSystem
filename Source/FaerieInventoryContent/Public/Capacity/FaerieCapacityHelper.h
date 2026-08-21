// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CapacityStructs.h"
#include "FaerieItemDataView.h"

namespace Faerie::ItemData
{
	/*
	 * Utility struct to interact with items that have Capacity.
	 */
	struct FAERIEINVENTORYCONTENT_API FCapacityHelper
	{
		UE_NONCOPYABLE(FCapacityHelper)

		FCapacityHelper(const FMassEntityManager* EntityManager UE_LIFETIMEBOUND, const FFaerieItemInstance& Instance);

		/*
		 * Adds capacity to the item if it doesn't have it.
		 * OverrideDefault will be used instead of the default if provided.
		 */
		void CreateCapacity(FMassEntityManager& InEntityManager, FFaerieItemInstance& Instance, const FFaerieItemCapacity* OverrideDefault = nullptr);
		void CreateCapacityIfMissing(FMassEntityManager& InEntityManager, FFaerieItemInstance& Instance, const FFaerieItemCapacity* OverrideDefault = nullptr);

		bool HasCapacity() const;
		FFaerieItemCapacity GetCapacity() const;

		bool HasDefaultCapacity() const;
		const FFaerieItemCapacity& GetDefaultCapacity() const;

		int32 GetWeightOfStack(const int32 Stack) const;

		// Gets the volume of an entire stack. Volume == X + (X * (Stack - 1) * Efficiency)
		int64 GetVolumeOfStack(const int32 Stack) const;

		// Gets the volume of a partial stack. Volume == X * Stack * Efficiency
		int64 GetEfficientVolume(const int32 Stack) const;

		// Gets the weight and volume of an entire stack.
		FFaerieWeightAndVolume GetWeightAndVolumeOfStack(const int32 Stack) const;

		// Gets the weight and volume for a portion of a stack. Uses EfficientVolume rather than full volume.
		FFaerieWeightAndVolume GetWeightAndVolumeOfPartialStack(const int32 Stack) const;

		void SetWeight(int32 NewValue);
		void SetBounds(const FIntVector& NewValue);
		void SetEfficiency(float NewValue);
		void SetCapacity(const FFaerieItemCapacity& NewValue);
		void ResetCapacity();

	private:
		const FMassEntityManager* EntityManager;
		const FFaerieItemInstance& Item;
		const FFaerieItemCapacity* MassCapacity = nullptr;
		const FFaerieItemCapacity* MassCapacityDefault = nullptr;
	};
}
