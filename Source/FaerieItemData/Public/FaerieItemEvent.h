// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "MassElement.h"

#include "Mass/EntityHandle.h"
#include "Mass/EntityElementTypes.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "FaerieItemEvent.generated.h"

#define FAE_API FAERIEITEMDATA_API

namespace Faerie::ItemData
{
	// Info packet for an item data mutation event. Carries the item handle that changed.
	USTRUCT()
	struct FMutationEvent : public FMassFragment
	{
		GENERATED_BODY()

		FMassEntityHandle ItemHandle;
		FGameplayTag EventType;

		// This is either the single FragmentType that was changed, or a ChangeList, in which case this event should be
		// bundled with a FMutationPayloadChangeList fragment.
		TWeakObjectPtr<const UScriptStruct> ChangeType;
	};

	USTRUCT()
	struct FMutationPayloadChangeList : public FMassFragment
	{
		GENERATED_BODY()

		UE::Mass::FElementBitSet ChangeTypes;
	};

	template<>
	struct TMassFragmentTraits<FMutationPayloadChangeList> final
	{
		enum
		{
			AuthorAcceptsItsNotTriviallyCopyable = true
		};
	};

	USTRUCT()
	struct FFieldChangePayload : public FMassFragment
	{
		GENERATED_BODY()

		FFieldChangePayload() = default;
		FFieldChangePayload(const uint16 Flags)
		  : ChangedFieldFlags(Flags) {}

	protected:
		// For now only supports 16 flags. But really, fragments should not have more than that many properties.
		uint16 ChangedFieldFlags = 0;

	public:
		void operator|=(const FFieldChangePayload Other)
		{
			ChangedFieldFlags |= Other.ChangedFieldFlags;
		}

		template <typename T>
		constexpr FFieldChangePayload& SetFlag(T Flag)
		{
			ChangedFieldFlags |= static_cast<uint16>(Flag);
			return *this;
		}

		template <typename T>
		constexpr FFieldChangePayload& ClearFlag(T Flag)
		{
			ChangedFieldFlags &= ~static_cast<uint16>(Flag);
			return *this;
		}

		template <typename T>
		constexpr bool HasFlag(T Flag) const
		{
			return (ChangedFieldFlags & static_cast<uint16>(Flag)) != 0;
		}
	};

	static constexpr FFieldChangePayload AllFields = FFieldChangePayload().SetFlag(TNumericLimits<uint16>::Max());

	// Event
	const FName EventCleanup = TEXT("ItemDataEventCleanup");
}

#undef FAE_API