// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemUsesToken.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFaerieRemainedUsesChanged);

USTRUCT()
struct FFaerieItemLastUseLogicBase
{
	GENERATED_BODY()

	virtual ~FFaerieItemLastUseLogicBase() = default;
  	virtual void OnLastUse(UFaerieItem* Item) const PURE_VIRTUAL(FFaerieItemLastUseLogicBase::OnLastUse, );
};

// Destroys the item when uses run out.
USTRUCT()
struct FFaerieItemLastUseLogic_Destroy : public FFaerieItemLastUseLogicBase
{
	GENERATED_BODY()

	virtual void OnLastUse(UFaerieItem* Item) const override;
};

class IFaerieItemSource;

// Swaps an item for a new instance on last use.
USTRUCT()
struct FFaerieItemLastUseLogic_Replace : public FFaerieItemLastUseLogicBase
{
	GENERATED_BODY()

	virtual void OnLastUse(UFaerieItem* Item) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "ItemPattern")
	TScriptInterface<IFaerieItemSource> BaseItemSource;
};

/**
 *
 */
UCLASS(DisplayName = "Token - Use Tracker")
class FAERIEITEMGENERATOR_API UFaerieItemUsesToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool IsMutable() const override { return true; }

	int32 GetMaxUses() const { return MaxUses; }
	int32 GetUsesRemaining() const { return UsesRemaining; }

	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	bool HasUses(int32 TestUses) const;

	/**
	 * Add uses to an entry.
	 */
	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	void AddUses(const int32 Amount, bool ClampRemainingToMax = true);

	/**
	 * Removes uses from an entry.
	 */
	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	bool RemoveUses(const int32 Amount);

	/**
	 * Resets remaining uses to value of max uses.
	 */
	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	void ResetUses();

	/**
	 * Removes uses from an entry
	 */
	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	void SetMaxUses(const int32 NewMax, bool ClampRemainingToMax = true);

protected:
	UFUNCTION(/* Replication */)
	void OnRep_UsesRemaining();

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "ItemUses", meta = (ExposeOnSpawn))
	int32 MaxUses;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_UsesRemaining, Category = "ItemUses", meta = (ExposeOnSpawn))
	int32 UsesRemaining;

	// Logic struct to run when the last use is removed. Replicates once on token creation.
	UPROPERTY(EditInstanceOnly, Replicated, Category = "ItemUses", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemLastUseLogicBase> LastUseLogic;

	UPROPERTY(BlueprintAssignable, Category = "Event")
	FFaerieRemainedUsesChanged OnUsesChanged;
};