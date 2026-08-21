// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieItemSource.h"

#include "FaerieItemUsesFragment.generated.h"

USTRUCT(meta = (Hidden))
struct FFaerieItemLastUseLogicBase : public FFaerieMassFragment
{
	GENERATED_BODY()

protected:
	/**
	 * We are making our own make-shift "vtable" here consisting of one single function pointer, because FFaerieMassFragment
	 * isn't allowed to have virtual children.
	 */
	void (*OnLastUseFuncPtr)(const FFaerieItemLastUseLogicBase*, FMassEntityManager&, const FFaerieItemProxy&, bool) = nullptr;

public:
	void HandleOnLastUse(const FFaerieItemLastUseLogicBase* ThisBase, FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, bool ProcessAsync) const;
};

// Destroys the item when uses run out.
USTRUCT()
struct FFaerieItemLastUseLogic_Destroy : public FFaerieItemLastUseLogicBase
{
	GENERATED_BODY()

	FFaerieItemLastUseLogic_Destroy();

	static void OnLastUse_Destroy(const FFaerieItemLastUseLogicBase* ThisBase, FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, bool ProcessAsync);
};

// Swaps an item for a new instance on last use.
USTRUCT()
struct FFaerieItemLastUseLogic_Replace : public FFaerieItemLastUseLogicBase
{
	GENERATED_BODY()

	FFaerieItemLastUseLogic_Replace();

	static void OnLastUse_Replace(const FFaerieItemLastUseLogicBase* ThisBase, FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, bool ProcessAsync);

protected:
	UPROPERTY(EditAnywhere, Category = "LastUseLogicReplace")
	FFaerieItemSourceObject BaseItemSource;
};

USTRUCT(BlueprintType)
struct FFaerieItemUses : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ItemUses")
	int32 MaxUses = 0;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "ItemUses")
	int32 UsesRemaining = 0;
};

namespace Faerie::ItemData
{
	template <>
	struct TMassFragmentTypeTraits<FFaerieItemUses> : TMassFragmentTypeTraitsBase<FFaerieItemUses>
	{
		enum
		{
			RequiresMutable = true
		};
	};

	/*
	 * Utility struct to interact with items that have uses.
	 */
	struct FAERIEITEMGENERATOR_API FUsesHelper : TFragmentHelperCRTP<FUsesHelper, FFaerieItemUses>
	{
		UE_NONCOPYABLE(FUsesHelper)

		FUsesHelper(const FMassEntityManager& EntityManager UE_LIFETIMEBOUND, const FFaerieItemInstance& Instance);

		/*
		 * Adds uses to the item if it doesn't have it.
		 * MaxUses will default to 1 if not provided.
		 * InitialUses will default to MaxUses if not provided.
		 */
		void CreateFragment(FMassEntityManager& EntityManager, FFaerieItemInstance& Instance, const TOptional<int32>& MaxUses = NullOpt, const TOptional<int32>& InitialUses = NullOpt);

		bool HasUsesRemaining(int32 Amount) const;

		void AddUses(int32 Amount, bool ClampToMax);

		/*
		 * Remove uses from item. Requires passing in proxy, as this function may need access to the owning container
		 * to handle last use logic. TEMPORARY UNTIL MASS ENTITY CAN FETCH OWNER!
		 */
		void RemoveUses(const FFaerieItemProxy& Proxy_TempForNow, int32 Amount);

		void SetUses(int32 Amount);

		void ResetUses();

		void SetMaxUses(int32 Value, bool ClampRemainingIfOverMax);

	private:
		const FMassEntityManager* EntityManager;
		const FFaerieItemInstance& Item;
	};
}
