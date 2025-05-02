// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieItemUsesToken.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFaerieRemainedUsesChanged);

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
	bool GetDestroyItemOnLastUse() const { return DestroyItemOnLastUse; }

	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	bool HasUses(int32 TestUses) const;

	/**
	 * SERVER ONLY.
	 * Add uses to an entry
	 */
	UFUNCTION(BlueprintAuthorityOnly, Category = "FaerieToken|Uses")
	void AddUses(const int32 Amount, bool ClampRemainingToMax = true);

	/**
	 * SERVER ONLY.
	 * Removes uses from an entry
	 */
	UFUNCTION(BlueprintAuthorityOnly, Category = "FaerieToken|Uses")
	bool RemoveUses(const int32 Amount);

	/**
	 * SERVER ONLY.
	 * Resets remaining uses to value of max uses.
	 */
	UFUNCTION(BlueprintAuthorityOnly, Category = "FaerieToken|Uses")
	void ResetUses();

	/**
	 * SERVER ONLY.
	 * Removes uses from an entry
	 */
	UFUNCTION(BlueprintAuthorityOnly, Category = "FaerieToken|Uses")
	void SetMaxUses(const int32 NewMax, bool ClampRemainingToMax = true);

protected:
	UFUNCTION(/* Replication */)
	void OnRep_UsesRemaining();

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "ItemUses", meta = (ExposeOnSpawn))
	int32 MaxUses;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_UsesRemaining, Category = "ItemUses", meta = (ExposeOnSpawn))
	int32 UsesRemaining;

	// Should this item be destroyed when its uses run out? Replicates once on token creation.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "ItemUses", meta = (ExposeOnSpawn))
	bool DestroyItemOnLastUse = true;

	UPROPERTY(BlueprintAssignable, Category = "Event")
	FFaerieRemainedUsesChanged OnUsesChanged;
};