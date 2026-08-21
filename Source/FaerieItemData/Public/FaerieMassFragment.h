// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "Mass/EntityElementTypes.h"
#include "FaerieItemDataConcepts.h"

#include "UObject/ObjectMacros.h"
#include "FaerieMassFragment.generated.h"

/*
 * Base type for Mass Structs that compose Item Entities.
 */
USTRUCT()
struct FAERIEITEMDATA_API FFaerieMassFragment : public FMassFragment
{
	GENERATED_BODY()
};

struct FMassEntityManager;

namespace Faerie::ItemData
{
	// Default enum values
	template <class TFaerieMassFragment>
	struct TMassFragmentTypeTraitsBase
	{
		enum
		{
			PrimaryIdentifier = false,
			RequiresMutable = false,
		};
	};

	// Template to override default enum values
	template <class TFragment>
	struct TMassFragmentTypeTraits final : TMassFragmentTypeTraitsBase<TFragment>
	{
	};

	// Concept to look for InitializeRuntime function on fragments types.
	template<typename T>
	concept CHasInitializeRuntime = requires(T& Value, TNotNull<UObject*> Outer, const FFaerieItemInstance& Instance)
	{
		{ Value.InitializeRuntime(Outer, Instance) } -> UE::CSameAs<bool>;
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
		template <CFragmentImpl TFragment>
		bool ExInitializeRuntime(TNotNull<void*> Value, TNotNull<UObject*> Outer, const FFaerieItemInstance& Instance)
		{
			if constexpr (CHasInitializeRuntime<TFragment>)
			{
				return static_cast<TFragment*>(NotNullGet(Value))->InitializeRuntime(Outer, Instance);
			}
			else
			{
				return false;
			}
		}

#if WITH_EDITOR
		template <CFragmentImpl TFragment>
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

	struct FMassFragmentTypeInterface
	{
		~FMassFragmentTypeInterface()
		{
			// @todo deleting this here causes issues. it should be deleted somewhere, but not here
			//delete Ops;
		}

		// @Todo this create a table of all pointers for every type, even if the struct does not implement the function.
		// This leaves a lot of pointers null, wasting a bit of memory per type.
		// The solution is to do what TStructOpsFakeVTable does, and generate a unique template permutation for each type.
		// This requires template magic I don't feel like
		struct ITraitOps
		{
			bool (*InitializeRuntimePtr)(TNotNull<void*>, TNotNull<UObject*>, const FFaerieItemInstance&) = nullptr;
#if WITH_EDITOR
			EDataValidationResult (*IsDataValidPtr)(const TNotNull<const void*>, class FDataValidationContext& Context) = nullptr;
#endif
		};

		template <CFragmentImpl TFragment>
		struct TTraitOps final : ITraitOps
		{
			TTraitOps()
			{
				InitializeRuntimePtr = &Private::ExInitializeRuntime<TFragment>;
#if WITH_EDITOR
				IsDataValidPtr = &Private::ExIsDataValid<TFragment>;
#endif
			}
		};

		bool PrimaryIdentifier; // @TODO implement filter by this
		bool RequiresMutable;
		bool HasInitializeRuntime;
#if WITH_EDITOR
		bool HasIsDataValid;
#endif
		const ITraitOps* Ops;

		bool InitializeRuntime(TNotNull<void*> Value, const TNotNull<UObject*> Outer, const FFaerieItemInstance& Instance) const
		{
			if (HasInitializeRuntime)
			{
				return Ops->InitializeRuntimePtr(Value, Outer, Instance);
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

	template <CFragmentImpl TFragment>
	FMassFragmentTypeInterface MakeTypeInterface()
	{
		using TraitsType = TMassFragmentTypeTraits<TFragment>;

		const FMassFragmentTypeInterface Interface
		{
			.PrimaryIdentifier = TraitsType::PrimaryIdentifier,
			.RequiresMutable = TraitsType::RequiresMutable,
			.HasInitializeRuntime = CHasInitializeRuntime<TFragment>,
#if WITH_EDITOR
			.HasIsDataValid = CHasIsDataValid<TFragment>,
#endif
			.Ops = new FMassFragmentTypeInterface::TTraitOps<TFragment>
		};

		return Interface;
	}

	struct IAutoRegisterFragmentTraits
	{
		FAERIEITEMDATA_API static void StaticRegisterTraits(TNotNull<const UScriptStruct*> Type, const FMassFragmentTypeInterface& Interface);
		FAERIEITEMDATA_API static void StaticDeregisterTraits(TNotNull<const UScriptStruct*> Type);
	};

	template <CFragmentImpl TFragment>
	struct TAutoRegisterFragmentTraits final : IAutoRegisterFragmentTraits, FDelayedAutoRegisterHelper
	{
		TAutoRegisterFragmentTraits()
			: FDelayedAutoRegisterHelper(EDelayedRegisterRunPhase::EndOfEngineInit, []()
			{
				IAutoRegisterFragmentTraits::StaticRegisterTraits(TFragment::StaticStruct(), MakeTypeInterface<TFragment>());
			}, true) {}

		~TAutoRegisterFragmentTraits()
		{
			IAutoRegisterFragmentTraits::StaticDeregisterTraits(TFragment::StaticStruct());
		}
	};

	FAERIEITEMDATA_API const FMassFragmentTypeInterface* GetFragmentTraitsInterface(TNotNull<const UScriptStruct*> Type);

	#define FAERIE_REGISTER_TRAITS(MassFragmentType) \
		[[maybe_unused]] static Faerie::ItemData::TAutoRegisterFragmentTraits<MassFragmentType> MassFragmentType##_TypeTraitsRegister;

	/*
	 * Template parent for implementing common functions for interacting with mass fragments on a faerie item instance.
	 */
	template<typename Impl, CFragmentImpl TFragment>
	struct [[nodiscard]] TFragmentHelperCRTP
	{
	private:
		UE_REWRITE Impl& AsImpl() { return static_cast<Impl&>(*this); }

	public:
		template <typename... ArgTypes>
		UE_REWRITE void CreateFragmentIfMissing(ArgTypes&&... InArgs)
		{
			if (!FragmentPtr)
			{
				AsImpl().CreateFragment(InArgs...);
			}
		}

		/*
		 * Does this instance has a live mass fragment registered.
		 */
		UE_REWRITE bool HasRuntimeFragment() const { return !!FragmentPtr; }

		/*
		 * Does this instance has a default value of this fragment type.
		 */
		UE_REWRITE bool HasDefaultFragment() const { return !!Defaults_FragmentPtr; }

		/*
		 * Does this instance has a live mass fragment registered or a default value of this fragment type.
		 */
		UE_REWRITE bool HasFragmentValue() const { return !!Defaults_FragmentPtr; }

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
		UE_REWRITE const TFragment* GetFragmentValue() const { return HasRuntimeFragment() ? FragmentPtr : Defaults_FragmentPtr; }

	protected:
		const TFragment* FragmentPtr = nullptr;
		const TFragment* Defaults_FragmentPtr = nullptr;
	};
}