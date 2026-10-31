// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "StructImplementationMacros.h"
#include "Mass/ExternalSubsystemTraits.h"

#include "FaerieMassFragment.generated.h"

#define FAE_API FAERIEITEMDATA_API

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

	/*
	 * Template parent for implementing common functions for interacting with mass fragments on a faerie item.
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
		 * Does this item have a live mass fragment registered.
		 */
		UE_REWRITE bool HasRuntimeValue() const { return !!FragmentPtr; }

		/*
		 * Does this item have a default value of this fragment type.
		 */
		UE_REWRITE bool HasDefaultValue() const { return !!Defaults_FragmentPtr; }

		/*
		 * Does this item have a live mass fragment registered or a default value of this fragment type.
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