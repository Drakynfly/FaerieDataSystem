// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemFilterEnums.h"
#include "FaerieItemProxy.h"

#include "Templates/SubclassOf.h"

#include "FaerieItemDataFilter.generated.h"

namespace Faerie::ItemData
{
#if WITH_EDITOR
	class FFilterLogger
	{
	public:
		TArray<FText> Errors;
	};
#endif
}

USTRUCT(meta = (Hidden))
struct FAERIEITEMDATA_API FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual ~FFaerieItemDataFilterBase() = default;

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const
		PURE_VIRTUAL(FFaerieItemDataFilterBase::Exec, return false; )

#if WITH_EDITOR
	// Overload with ability to log errors. Used by editor validation to collect info about failures.
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const;

	// This function allows the owning object to know if this filter will allow mutable or immutable assets through, or
	// if it doesn't know. The default is unknown, and specific children must override one way or the other.
	// This function is only called in the editor and saved to a variable when needed at runtime.
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const { return EFaerieItemDataMutabilityStatus::Unknown; }
#endif
};

UCLASS(Abstract, Const, Blueprintable)
class UFaerieItemDataFilter_BlueprintBase final : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, meta = (DisplayName = "Execute"))
	bool BP_Execute(const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent)
	EFaerieItemDataMutabilityStatus GetMutabilityStatus() const;
};

USTRUCT()
struct FFaerieItemDataFilter_Blueprint final : public FFaerieItemDataFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemDataMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Blueprint Filter")
	TSubclassOf<UFaerieItemDataFilter_BlueprintBase> Blueprint;
};