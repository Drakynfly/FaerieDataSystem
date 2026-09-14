// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemProxy.h"

#include "Templates/SubclassOf.h"

#include "FaerieItemFilter.generated.h"

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

UENUM()
enum class EFaerieItemFilterMutabilityStatus : uint8
{
	Unknown,
	KnownMutable,
	KnownImmutable,
	Conflict
};

namespace Faerie::ItemData
{
	inline EFaerieItemFilterMutabilityStatus CombineStatuses(const EFaerieItemFilterMutabilityStatus A, const EFaerieItemFilterMutabilityStatus B)
	{
		if (A == EFaerieItemFilterMutabilityStatus::Conflict || B == EFaerieItemFilterMutabilityStatus::Conflict)
		{
			return EFaerieItemFilterMutabilityStatus::Conflict;
		}

		if (A == EFaerieItemFilterMutabilityStatus::Unknown) return B;
		if (B == EFaerieItemFilterMutabilityStatus::Unknown) return A;

		if (A == B)
		{
			return A;
		}

		return EFaerieItemFilterMutabilityStatus::Conflict;
	}
}

USTRUCT(meta = (Hidden))
struct FAERIEITEMDATA_API FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual ~FFaerieItemFilterBase() = default;

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const
		PURE_VIRTUAL(FFaerieItemFilterBase::Exec, return false; )

#if WITH_EDITOR
	// Overload with ability to log errors. Used by editor validation to collect info about failures.
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const;

	// This function allows the owning object to know if this filter will allow mutable or immutable assets through, or
	// if it doesn't know. The default is unknown, and specific children must override one way or the other.
	// This function is only called in the editor and saved to a variable when needed at runtime.
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const { return EFaerieItemFilterMutabilityStatus::Unknown; }
#endif
};

UCLASS(Abstract, Const, Blueprintable)
class UFaerieItemFilter_BlueprintBase final : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, meta = (DisplayName = "Execute"))
	bool BP_Execute(const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent)
	EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const;
};

USTRUCT()
struct FFaerieItemFilter_Blueprint final : public FFaerieItemFilterBase
{
	GENERATED_BODY()

	virtual bool Exec(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;

#if WITH_EDITOR
	virtual bool ExecWithLog(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> Proxy, Faerie::ItemData::FFilterLogger& Logger) const override;
	virtual EFaerieItemFilterMutabilityStatus GetMutabilityStatus() const override;
#endif

	UPROPERTY(EditAnywhere, Category = "Blueprint Filter")
	TSubclassOf<UFaerieItemFilter_BlueprintBase> Blueprint;
};