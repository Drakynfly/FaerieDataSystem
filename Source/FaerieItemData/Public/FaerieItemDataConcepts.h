// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataFwd.h"
#include "MassEntityConcepts.h"
#include "Templates/RemoveReference.h"
#include "Templates/UnrealTypeTraits.h"

namespace Faerie::ItemData
{
	template<typename T>
	using Clean = UE::Mass::Clean<T>;

	// Concept for a faerie fragment type.
	template <typename T>
	concept CFragmentType =
		TIsDerivedFrom<Clean<T>, FFaerieMassFragment>::Value ||
		TIsDerivedFrom<Clean<T>, FFaerieMassSparseFragment>::Value;

	// Concept for a faerie fragment base type.
	template <typename T>
	concept CFragmentBase = UE::CSameAs<Clean<T>, FFaerieMassFragment> || UE::CSameAs<Clean<T>, FFaerieMassSparseFragment>;

	// Concept for an implementation of a faerie fragment type.
	template <typename T>
	concept CFragmentImpl = CFragmentType<T> && !CFragmentBase<T>;


	// Concept for a faerie tag type.
	template <typename T>
	concept CTagType = TIsDerivedFrom<Clean<T>, FFaerieMassTag>::Value || TIsDerivedFrom<Clean<T>, FFaerieMassSparseTag>::Value;

	// Concept for a faerie tag base type.
	template <typename T>
	concept CTagBase =
		UE::CSameAs<Clean<T>, FFaerieMassTag> || UE::CSameAs<Clean<T>, FFaerieMassSparseTag>;

	// Concept for an implementation of a faerie tag type.
	template <typename T>
	concept CTagImpl = CTagType<T> && !CTagBase<T>;


	// Concept for an implementation of a faerie shared type.
	template <typename T>
	concept CSharedImpl = TIsDerivedFrom<Clean<T>, FFaerieMassSharedFragment>::Value && !UE::CSameAs<Clean<T>, FFaerieMassSharedFragment>;

	// Concept for an implementation of a faerie shared type.
	template <typename T>
	concept CConstSharedImpl = TIsDerivedFrom<Clean<T>, FFaerieMassConstSharedFragment>::Value && !UE::CSameAs<Clean<T>, FFaerieMassConstSharedFragment>;


	// Concept for an implementation of a faerie element type that holds data.
	template <typename T>
	concept CDataImpl = CFragmentImpl<T> || CSharedImpl<T> || CConstSharedImpl<T>;

	// Concept for an implementation of any faerie element type.
	template <typename T>
	concept CElementImpl = CFragmentImpl<T> || CTagImpl<T> || CSharedImpl<T> || CConstSharedImpl<T>;

	template <typename T>
	concept CItemDataProxy = TIsDerivedFrom<Clean<T>, IFaerieItemDataProxy>::Value;
}