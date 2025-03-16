// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemSource.h"

#include "FaerieItemAsset.generated.h"

class UFaerieItem;
class UFaerieItemToken;
class UFaerieItemTemplate;

/**
 * A basic item definition. Used to generate generic items with no procedural data.
 */
UCLASS(BlueprintType, Const)
class FAERIEITEMDATA_API UFaerieItemAsset : public UObject, public IFaerieItemSource
{
	GENERATED_BODY()

public:
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	//~ IFaerieItemSource
	virtual bool CanBeMutable() const override;
	virtual FFaerieAssetInfo GetSourceInfo() const override;
	virtual UFaerieItem* CreateItemInstance(const UItemInstancingContext* Context) const override;
	//~ IFaerieItemSource

	// Get the item instance this asset represents. By default, this will return the immutable asset if possible.
	// If the item needs to allow changes, enable MutableInstance.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemAsset")
	UFaerieItem* GetItemInstance(EFaerieItemInstancingMutability Mutability) const;

#if WITH_EDITOR
	// Gets a const ptr to the archetype item this asset generates.
	// ONLY AVAILABLE IN EDITOR!
	const UFaerieItem* GetEditorItemView() const { return Item; }
	TConstArrayView<TObjectPtr<UFaerieItemToken>> GetEditorTokensView() const { return Tokens; }
#endif

protected:
	UPROPERTY(VisibleAnywhere, DuplicateTransient, Category = "ItemAsset")
	TObjectPtr<UFaerieItem> Item;

#if WITH_EDITORONLY_DATA
	// Tokens used to build the Item. Only exist in the editor, as the item is compiled by PreSave.
	UPROPERTY(EditInstanceOnly, Instanced, Category = "ItemAsset")
	TArray<TObjectPtr<UFaerieItemToken>> Tokens;

	// Item template, used to verify that the generated item follows an expected pattern.
	UPROPERTY(EditInstanceOnly, Category = "ItemAsset")
	TObjectPtr<UFaerieItemTemplate> Template;

	// Enable to make all instances of this item mutable, even if no current Tokens request mutability. This is usually
	// required when making an item template expected to have a mutable token added dynamically at runtime, but
	// doesn't have any mutable tokens added by the editor.
	UPROPERTY(EditInstanceOnly, Category = "ItemAsset")
	bool AlwaysMutable = false;
#endif
};