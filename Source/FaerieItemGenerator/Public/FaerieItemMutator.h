// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/SoftObjectPtr.h"

#include "FaerieItemGeneratorModule.h" // Needed for FAERIE_IMPL_StructTypeCustomization macro
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

// Macro to semi-automate implementation of virtual struct machinery.
#define FAERIE_IMPL_GetScriptStruct() public: virtual const UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

#if WITH_EDITOR
// Declare editor-only type customization auto register RAII static.
#define FAERIE_IMPL_StructTypeCustomization(Type)\
	[[maybe_unused]] static Faerie::Generation::TMutatorStructTypeCustomizationAutoRegister<Type> Type##_CustomizationRegister;
#else
#define FAERIE_IMPL_StructTypeCustomization(Type)
#endif

#define FAERIE_IMPL_TStructOpsTypeTraits(Type)\
template<> struct TStructOpsTypeTraits<Type> : public TStructOpsTypeTraitsBase2<Type>\
{\
	enum\
	{\
		WithPostSerialize = true, \
	};

#define FAERIE_MUTATOR_HEADER(Type)\
	FAERIE_IMPL_GetScriptStruct()\
	};\
	FAERIE_IMPL_TStructOpsTypeTraits(Type)

#define FAERIE_MUTATOR_IMPL(Type)\
	FAERIE_IMPL_StructTypeCustomization(Type)

template<>
struct TStructOpsTypeTraits<FFaerieItemMutator> : public TStructOpsTypeTraitsBase2<FFaerieItemMutator>
{
	enum
	{
		WithPostSerialize = true,
	};
};
