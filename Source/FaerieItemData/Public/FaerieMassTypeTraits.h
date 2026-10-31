// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "MassEntityManager.h"

#define FAE_API FAERIEITEMDATA_API

namespace Faerie::ItemData
{
	// Default trait values for a mass fragment.
	template <class TFaerieMassFragment>
	struct TMassTypeTraitsBase
	{
		enum
		{
			// Marks the fragment as being used to tell items apart. @todo actually implement this lol
			PrimaryIdentifier = false,

			// The fragment will force the item to be considered mutable. This disables stacking, and enables runtime mutation.
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

	// Macro to implement a type traits interface. Must be placed in a cpp file, for each type that implements one of the
	// detected vtable functions or overrides one of the trait flags. Does not need to be used for children that want to
	// keep their parent's interface.
#define FAERIE_REGISTER_TRAITS(Type) \
	namespace\
	{\
		[[maybe_unused]] Faerie::ItemData::TAutoRegisterMassTraits<Type> Type##_TypeTraitsRegister;\
	}
}

#undef FAE_API