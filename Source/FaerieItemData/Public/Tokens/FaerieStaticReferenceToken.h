// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieStaticReferenceToken.generated.h"

class UFaerieItemAsset;

USTRUCT()
struct FFaerieTaggedStaticReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "TaggedStaticReference")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, Category = "TaggedStaticReference")
	TObjectPtr<const UFaerieItemAsset> Reference;
};

/**
 * This token allows an item to reference other item assets, and get their tokens.
 */
UCLASS(meta = (DisplayName = "Token - Static Reference"))
class FAERIEITEMDATA_API UFaerieStaticReferenceToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	static UFaerieStaticReferenceToken* CreateInstance(TConstArrayView<FFaerieTaggedStaticReference> InReferences);

	static const UFaerieItem* GetReferencedItem(const UFaerieItem& Item, FGameplayTag ReferenceTag, bool MatchExact);

	UFUNCTION(BlueprintCallable, Category = "Faerie|StaticReferenceToken")
	const UFaerieItem* GetReferencedItem(FGameplayTag ReferenceTag, bool MatchExact) const;

protected:
	UPROPERTY(EditAnywhere, Replicated, Category = "StaticReferenceToken", meta = (ForceInlineRow))
	TArray<FFaerieTaggedStaticReference> References;
};

namespace Faerie
{
	// Gets a view of all tokens for a reference.
	FAERIEITEMDATA_API TConstArrayView<TObjectPtr<UFaerieItemToken>> GetReferencedTokens(const UFaerieItem& Item, FGameplayTag ReferenceTag, bool MatchExact = false);

	// Gets the token at a specified index of a reference. Low-level access for when you know what you are doing.
	FAERIEITEMDATA_API const UFaerieItemToken* GetReferencedTokenAtIndex(const UFaerieItem& Item, int32 Index, FGameplayTag ReferenceTag, bool MatchExact = false);

	// Gets the first token of the specified class.
	FAERIEITEMDATA_API const UFaerieItemToken* GetReferencedToken(const UFaerieItem& Item, const TSubclassOf<UFaerieItemToken>& Class, FGameplayTag ReferenceTag, bool MatchExact = false);

	// Gets the first token of the specified class.
	template <CItemTokenImpl T>
	const T* GetReferencedToken(const UFaerieItem& Item, FGameplayTag ReferenceTag, bool MatchExact = false)
	{
		return CastChecked<T>(GetReferencedToken(Item, T::StaticClass(), ReferenceTag, MatchExact), ECastCheckedType::NullAllowed);
	}
}
