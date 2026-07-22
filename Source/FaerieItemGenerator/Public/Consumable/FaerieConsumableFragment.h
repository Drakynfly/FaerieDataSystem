// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "Templates/SubclassOf.h"
#include "FaerieConsumableFragment.generated.h"

namespace Faerie::Generation
{
	FAERIEITEMGENERATOR_API bool CanConsume(const FFaerieItemProxy& Proxy, TNotNull<const UScriptStruct*> FragmentType, const TNotNull<const AActor*> Consumer, const int32 Cost);
	FAERIEITEMGENERATOR_API bool TryConsume(const FFaerieItemProxy& Proxy, TNotNull<const UScriptStruct*> FragmentType, const TNotNull<AActor*> Consumer, const int32 Cost);

	FAERIEITEMGENERATOR_API bool CanRemoveUses(const FFaerieItemProxy& Proxy, const ItemData::FRequireEntityManager& EntityManager, int32 Cost, bool ResultIfNoUsesFragment);
	FAERIEITEMGENERATOR_API void RemoveUses(const FFaerieItemProxy& Proxy, const ItemData::FRequireEntityManager& EntityManager, int32 Cost);
}

// Extension point for implementation of consumption logic.
UCLASS(Abstract, BlueprintType, Const)
class FAERIEITEMGENERATOR_API UFaerieConsumableLogicBase : public UObject
{
	GENERATED_BODY()

public:
	virtual bool TestConsumable(const TConstStructView<FFaerieMassFragment>& Fragment, const FFaerieItemProxy& Proxy, TNotNull<const AActor*> Consumer, int32 Cost) const;

	virtual void OnConsumed(const TConstStructView<FFaerieMassFragment>& Fragment, const FFaerieItemProxy& Proxy, TNotNull<AActor*> Consumer, int32 Cost) const
		PURE_VIRTUAL(UFaerieConsumableLogicBase::OnConsumed, )
};

USTRUCT(BlueprintType)
struct FFaerieConsumableFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Consumable")
	TSubclassOf<UFaerieConsumableLogicBase> LogicClass;

	const UFaerieConsumableLogicBase* GetConsumableLogic() const { return LogicClass.GetDefaultObject(); }
};