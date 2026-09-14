// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/SoftObjectPtr.h"
#include "Containers/AdderRef.h"

#include "FaerieItemGeneratorModule.h" // Needed for FAERIE_IMPL_MutatorStructTypeCustomization macro
#include "StructImplementationMacros.h"

#include "FaerieItemMutator.generated.h"

struct FFaerieItemInstance;
struct FMassEntityManager;
class USquirrel;

USTRUCT()
struct FFaerieItemMutatorContext
{
	GENERATED_BODY()

	virtual ~FFaerieItemMutatorContext() = default;

	// Entity manager used to access runtime item fragments.
	FMassEntityManager* EntityManager = nullptr;

	UPROPERTY()
	TObjectPtr<USquirrel> Squirrel;

#if WITH_EDITORONLY_DATA
	// A flag to mark a mutator context as being run by the editor.
	bool RunningInEditor = false;
#endif

	// Children must implement this to allow safe casting.
	UE_REWRITE virtual const UScriptStruct* GetScriptStruct() const { return FFaerieItemMutatorContext::StaticStruct(); }

	template <typename T>
	const T* Cast() const
	{
		if (GetScriptStruct()->IsChildOf<T>())
		{
			return static_cast<const T*>(this);
		}
		return nullptr;
	}

	template <typename T>
	T* Cast()
	{
		if (GetScriptStruct()->IsChildOf<T>())
		{
			return static_cast<T*>(this);
		}
		return nullptr;
	}
};

/**
 * Base struct for mutation behavior. This functions as a 'command' class, with some helpers to get loading data.
 * GetRequiredAssets() is optional to implement.
 * Apply() must be implemented.
 */
USTRUCT()
struct FAERIEITEMGENERATOR_API FFaerieItemMutator
{
	GENERATED_BODY()

	virtual ~FFaerieItemMutator() = default;

	void PostSerialize(const FArchive& Ar);

	virtual const UScriptStruct* GetScriptStruct() const PURE_VIRTUAL(FFaerieItemMutator::GetScriptStruct, return nullptr; )

	// Any soft assets required to be loaded when Apply is called should be registered here.
	virtual void GetRequiredAssets(TAdderRef<FSoftObjectPath> RequiredAssets) const {}

	// Try to run this mutator on a stack.
	virtual bool Apply(FFaerieItemInstance& Item, const FFaerieItemMutatorContext& Context) const PURE_VIRTUAL(FFaerieItemMutator::Apply, return false; )
};

template<>
struct TStructOpsTypeTraits<FFaerieItemMutator> : public TStructOpsTypeTraitsBase2<FFaerieItemMutator>
{
	enum
	{
		WithPostSerialize = true,
	};
};

#if WITH_EDITOR
// Declare editor-only type customization auto register RAII.
#define FAERIE_IMPL_MutatorStructTypeCustomization(Type)\
namespace\
{\
	[[maybe_unused]] Faerie::Generation::TMutatorStructTypeCustomizationAutoRegister<Type> Type##_CustomizationRegister;\
}
#else
#define FAERIE_IMPL_MutatorStructTypeCustomization(Type)
#endif

// Macro to implement a faerie mutator type. Place in header at end of struct declaration.
#define ____FAERIE_MUTATOR_DECL(Type)\
	FAERIE_IMPL_GetScriptStruct()\
	};\
	FAERIE_IMPL_TStructOpsTypeTraits_BEGIN(Type)\
	WithPostSerialize = true,\
	FAERIE_IMPL_TStructOpsTypeTraits_END(Type)

// Macro to implement a faerie mutator type. Place in cpp file.
#define FAERIE_MUTATOR_IMPL(Type)\
	FAERIE_IMPL_MutatorStructTypeCustomization(Type)