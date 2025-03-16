// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieItemUsesToken.generated.h"

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

	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	int32 GetUsesRemaining() const { return UsesRemaining; }

	UFUNCTION(BlueprintCallable, Category = "FaerieToken|Uses")
	int32 GetMaxUses() const { return MaxUses; }

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
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "ItemUses", meta = (ExposeOnSpawn))
	int32 MaxUses;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "ItemUses", meta = (ExposeOnSpawn))
	int32 UsesRemaining;
};