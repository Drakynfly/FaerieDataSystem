// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "Mass/EntityHandle.h"
#include "StructImplementationMacros.h"
#include "Mass/ExternalSubsystemTraits.h"

#include "UObject/ObjectMacros.h"
#include "FaerieMassFragment.generated.h"

#define FAE_API FAERIEITEMDATA_API

namespace UE::Mass
{
	struct FEntityBuilder;
}

struct FInstancedStruct;

/*
 * Faerie Mass Types have Serialize overridden to export the type as searchable.
 * Normally you are supposed to do this inside PostSerialize, but we don't have access to our own ScriptStruct from
 * that function. Other base structs can implement a virtual GetScriptStruct getter function, but fragments are not
 * allowed to be virtual, so we cannot use that option.
 * However, Serialize has an alternate version detected by CStructSerializableWithDefaults that passes in our
 * UStruct reflection type.
 */

// ReSharper disable CppUEMissingStructMember
#define FAERIE_MASS_FRAGMENT_BODY()\
	FAE_API bool Serialize(FArchive& Ar, const UStruct* DefaultsStruct, const void* Defaults);

// Macro to implement a faerie fragment type. Place in header at end of struct declaration.
#define ____FAERIE_FRAGMENT_DECL(Type)\
	};\
	FAERIE_IMPL_TStructOpsTypeTraits_BEGIN(Type)\
	WithSerializer = true,\
	FAERIE_IMPL_TStructOpsTypeTraits_END(Type)

/*
 * Base type for Mass Structs that compose Item Entities.
 */
USTRUCT()
// Disabling ReSharper error, because it doesn't detect the Serialize overload with extra args.
struct FFaerieMassFragment : public FMassFragment
{
	GENERATED_BODY()
	FAERIE_MASS_FRAGMENT_BODY()
____FAERIE_FRAGMENT_DECL(FFaerieMassFragment)
};

USTRUCT()
struct FFaerieMassSparseFragment : public FMassSparseFragment
{
	GENERATED_BODY()
	FAERIE_MASS_FRAGMENT_BODY()
____FAERIE_FRAGMENT_DECL(FFaerieMassSparseFragment)
};

USTRUCT()
struct FFaerieMassTag : public FMassTag
{
	GENERATED_BODY()
	FAERIE_MASS_FRAGMENT_BODY()
____FAERIE_FRAGMENT_DECL(FFaerieMassTag)
};

USTRUCT()
struct FFaerieMassSparseTag : public FMassSparseTag
{
	GENERATED_BODY()
	FAERIE_MASS_FRAGMENT_BODY()
____FAERIE_FRAGMENT_DECL(FFaerieMassSparseTag)
};

USTRUCT()
struct FFaerieMassSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()
	FAERIE_MASS_FRAGMENT_BODY()
____FAERIE_FRAGMENT_DECL(FFaerieMassSharedFragment)
};

USTRUCT()
struct FFaerieMassConstSharedFragment : public FMassConstSharedFragment
{
	GENERATED_BODY()
	FAERIE_MASS_FRAGMENT_BODY()
____FAERIE_FRAGMENT_DECL(FFaerieMassConstSharedFragment)
};

// Base type for exported save data for an item fragment. This is not usually needed as most fragment types can be used
// as is, as long as their members are UProperties. If a Fragment does need a custom export format, then subclass this,
// override GetFragmentType to return the mass type's StaticStruct, and implement SaveDataConverter on the fragment type.
// See FFaerieItemStorageFragment for an example of this.
USTRUCT()
struct FFaerieItemFragmentSaveData
{
	GENERATED_BODY()

	virtual ~FFaerieItemFragmentSaveData() = default;
	virtual const UScriptStruct* GetFragmentType() const PURE_VIRTUAL(FFaerieItemFragmentSaveData::GetFragmentType, return nullptr; )
};

struct FMassEntityManager;

namespace Faerie::ItemData
{
	FAE_API bool IsFaerieMassType(TNotNull<const UScriptStruct*> Type);
	FAE_API bool IsFaerieMassTag(TNotNull<const UScriptStruct*> Type);

	// Default trait values for a mass fragment.
	template <class TFaerieMassFragment>
	struct TMassTypeTraitsBase
	{
		enum
		{
			// Marks the fragment as being used to tell instances apart. @todo actually implement this lol
			PrimaryIdentifier = false,

			// The fragment will force the instance to be considered mutable. This disabled stacking, and enables runtime mutation.
			RequiresMutable = false,

			// The fragment is exported for serialization to save data. Disable for transient data.
			Serialized = true,

			// The fragment is exported for network replication. Disable for locally recreated data.
			Replicated = true,
		};
	};

	// Template to override default enum values
	template <class TFragment>
	struct TMassTypeTraits final : TMassTypeTraitsBase<TFragment>
	{
	};

	// Concept to look for InitializeRuntime function on fragments types.
	template<typename T>
	concept CHasInitializeRuntime = requires(T& Value, UE::Mass::FEntityBuilder& Builder)
	{
		{ Value.InitializeRuntime(Builder) } -> UE::CSameAs<bool>;
	};

	// Concept to look for SaveDataConverter function on fragments types.
	template<typename T>
	concept CHasSaveDataConverter = requires(T& Value, FMassEntityManager& EntityManager, FInstancedStruct& SaveData)
	{
		{ Value.SaveDataConverter(EntityManager, SaveData) } -> UE::CSameAs<bool>;
	};

#if WITH_EDITOR
	// Concept to look for IsDataValid function on fragments types.
	template<typename T>
	concept CHasIsDataValid = requires(const T& Value, class FDataValidationContext& Context)
	{
		{ Value.IsDataValid(Context) } -> UE::CSameAs<EDataValidationResult>;
	};
#endif

	namespace Private
	{
		template <CDataImpl TFragment>
		bool ExInitializeRuntime(TNotNull<void*> Value, UE::Mass::FEntityBuilder& Builder)
		{
			if constexpr (CHasInitializeRuntime<TFragment>)
			{
				return static_cast<TFragment*>(NotNullGet(Value))->InitializeRuntime(Builder);
			}
			else
			{
				return false;
			}
		}

		template <CDataImpl TFragment>
		bool ExSaveDataConverter(TNotNull<void*> Value, FMassEntityManager& EntityManager, FInstancedStruct& SaveData)
		{
			if constexpr (CHasSaveDataConverter<TFragment>)
			{
				return static_cast<TFragment*>(NotNullGet(Value))->SaveDataConverter(EntityManager, SaveData);
			}
			else
			{
				return false;
			}
		}

#if WITH_EDITOR
		template <CDataImpl TFragment>
		EDataValidationResult ExIsDataValid(const TNotNull<const void*> Value, class FDataValidationContext& Context)
		{
			if constexpr (CHasIsDataValid<TFragment>)
			{
				return static_cast<const TFragment*>(NotNullGet(Value))->IsDataValid(Context);
			}
			else
			{
				return EDataValidationResult::NotValidated;
			}
		}
#endif
	}

	struct FMassTypeTraitsInterface
	{
		~FMassTypeTraitsInterface()
		{
			// @todo deleting this here causes issues. it should be deleted somewhere, but not here
			//delete Ops;
		}

		// @Todo this create a table of all pointers for every type, even if the struct does not implement the function.
		// This leaves a lot of pointers null, wasting a bit of memory per type.
		// The solution is to do what TStructOpsFakeVTable does, and generate a unique template permutation for each type.
		// This requires template magic I don't feel like writing currently.
		struct ITraitOps
		{
			bool (*InitializeRuntimePtr)(TNotNull<void*>, UE::Mass::FEntityBuilder&) = nullptr;
			bool (*SaveDataConverterPtr)(TNotNull<void*>, FMassEntityManager&, FInstancedStruct&) = nullptr;
#if WITH_EDITOR
			EDataValidationResult (*IsDataValidPtr)(const TNotNull<const void*>, class FDataValidationContext& Context) = nullptr;
#endif
		};

		template <CElementImpl TElement>
		struct TTraitOps final : ITraitOps
		{
			TTraitOps()
			{
				InitializeRuntimePtr = &Private::ExInitializeRuntime<TElement>;
				SaveDataConverterPtr = &Private::ExSaveDataConverter<TElement>;
#if WITH_EDITOR
				IsDataValidPtr = &Private::ExIsDataValid<TElement>;
#endif
			}
		};

		bool PrimaryIdentifier; // @TODO implement filter by this
		bool RequiresMutable;
		bool Serialized;
		bool Replicated;
		bool HasInitializeRuntime;
		bool HasSaveDataConverter;
#if WITH_EDITOR
		bool HasIsDataValid;
#endif
		const ITraitOps* Ops;

		bool InitializeRuntime(TNotNull<void*> Value, UE::Mass::FEntityBuilder& Builder) const
		{
			if (HasInitializeRuntime)
			{
				return Ops->InitializeRuntimePtr(Value, Builder);
			}
			return false;
		}

		bool SaveDataConverter(TNotNull<void*> Value, FMassEntityManager& EntityManager, FInstancedStruct& SaveData) const
		{
			if (HasSaveDataConverter)
			{
				return Ops->SaveDataConverterPtr(Value, EntityManager, SaveData);
			}
			return false;
		}

#if WITH_EDITOR
		EDataValidationResult IsDataValid(const TNotNull<const void*> Value, class FDataValidationContext& Context) const
		{
			if (HasIsDataValid)
			{
				return Ops->IsDataValidPtr(Value, Context);
			}
			return EDataValidationResult::NotValidated;
		}
#endif
	};

	template <CElementImpl TElement>
	FMassTypeTraitsInterface MakeTypeInterface()
	{
		using TraitsType = TMassTypeTraits<TElement>;

		const FMassTypeTraitsInterface Interface
		{
			.PrimaryIdentifier = TraitsType::PrimaryIdentifier,
			.RequiresMutable = TraitsType::RequiresMutable,
			.Serialized = TraitsType::Serialized,
			.Replicated = TraitsType::Replicated,
			.HasInitializeRuntime = CHasInitializeRuntime<TElement>,
			.HasSaveDataConverter = CHasSaveDataConverter<TElement>,
#if WITH_EDITOR
			.HasIsDataValid = CHasIsDataValid<TElement>,
#endif
			.Ops = new FMassTypeTraitsInterface::TTraitOps<TElement>
		};

		return Interface;
	}

	struct IAutoRegisterMassTraits
	{
		FAE_API static void StaticRegisterTraits(TNotNull<const UScriptStruct*> Type, const FMassTypeTraitsInterface& Interface);
		FAE_API static void StaticDeregisterTraits(TNotNull<const UScriptStruct*> Type);
	};

	template <CElementImpl TElement>
	struct TAutoRegisterMassTraits final : FDelayedAutoRegisterHelper
	{
		TAutoRegisterMassTraits()
		  : FDelayedAutoRegisterHelper(EDelayedRegisterRunPhase::EndOfEngineInit, []()
			{
				IAutoRegisterMassTraits::StaticRegisterTraits(TElement::StaticStruct(), MakeTypeInterface<TElement>());
			}, true) {}

		~TAutoRegisterMassTraits()
		{
			IAutoRegisterMassTraits::StaticDeregisterTraits(TElement::StaticStruct());
		}
	};

	FAE_API const FMassTypeTraitsInterface* GetMassTraitsInterface(TNotNull<const UScriptStruct*> Type);

#define FAERIE_REGISTER_TRAITS(Type) \
	namespace\
	{\
		[[maybe_unused]] Faerie::ItemData::TAutoRegisterMassTraits<Type> Type##_TypeTraitsRegister;\
	}

	/*
	 * Template parent for implementing common functions for interacting with mass fragments on a faerie item instance.
	 */
	template<typename Impl, CDataImpl TFragment>
	struct [[nodiscard]] TFragmentHelperCRTP
	{
	private:
		UE_REWRITE Impl& AsImpl() { return static_cast<Impl&>(*this); }

	public:
		template <typename... ArgTypes>
		UE_REWRITE Impl& CreateFragmentIfMissing(ArgTypes&&... InArgs)
		{
			if (!FragmentPtr)
			{
				check(Defaults_FragmentPtr)
				AsImpl().CreateFragment(InArgs...);
			}
			return AsImpl();
		}

		/*
		 * Does this instance has a live mass fragment registered.
		 */
		UE_REWRITE bool HasRuntimeValue() const { return !!FragmentPtr; }

		/*
		 * Does this instance has a default value of this fragment type.
		 */
		UE_REWRITE bool HasDefaultValue() const { return !!Defaults_FragmentPtr; }

		/*
		 * Does this instance has a live mass fragment registered or a default value of this fragment type.
		 */
		UE_REWRITE bool HasFragmentValue() const { return HasRuntimeValue() || HasDefaultValue(); }

		/*
		 * Get the value of the runtime fragment. Assumes validity.
		 */
		UE_REWRITE const TFragment& GetRuntimeValue() const { return *FragmentPtr; }

		/*
		 * Get the value of the default fragment. Assumes validity.
		 */
		UE_REWRITE const TFragment& GetDefaultValue() const { return *Defaults_FragmentPtr; }

		/*
		 * Get the live mass fragment if it exists or the default otherwise.
		 */
		UE_REWRITE const TFragment* GetFragmentValue() const { return HasRuntimeValue() ? FragmentPtr : Defaults_FragmentPtr; }

	protected:
		const TFragment* FragmentPtr = nullptr;
		const TFragment* Defaults_FragmentPtr = nullptr;
	};
}

#undef FAE_API