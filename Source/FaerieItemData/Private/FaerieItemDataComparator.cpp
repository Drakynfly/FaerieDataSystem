// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataComparator.h"
#include "EntityManagerHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataComparator)

bool UFaerieItemDataComparator::K2_Exec(const FFaerieItemProxy& A, const FFaerieItemProxy& B) const
{
	auto* EntityManager = Faerie::ItemData::GetFaerieEntityManager();

	if (A.IsValid() && B.IsValid())
	{
		return Exec(EntityManager, A, B);
	}
	return false;
}

bool UFaerieItemDataComparator_BlueprintBase::Exec(const FMassEntityManager*,
												   const Faerie::TValid<const FFaerieItemProxy&> ProxyA,
												   const Faerie::TValid<const FFaerieItemProxy&> ProxyB) const
{
	return Execute(ValidGet(ProxyA), ValidGet(ProxyB));
}