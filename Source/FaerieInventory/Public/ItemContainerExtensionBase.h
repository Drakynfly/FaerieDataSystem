// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ArrayAdapter.h"
#include "FaerieInventoryConcepts.h"
#include "FaerieInventoryTag.h"
#include "FaerieStorageEnums.h"
#include "FaerieUnownedItemStack.h"
#include "NetSupportedObject.h"
#include "ValidParameter.h"

#include "StructImplementationMacros.h"

#include "ItemContainerExtensionBase.generated.h"

namespace Faerie::Container
{
	class IAddressView;
}

UENUM()
enum class EFaerieExtensionResponse : uint8
{
	// The extension does not care/have authority to allow or deny the event.
	NoExplicitResponse,

	// The extension allows the event
	Allowed,

	// The extension forbids the event
	Disallowed
};

USTRUCT(BlueprintType)
struct FFaerieExtensionAllowsAdditionArgs
{
	GENERATED_BODY()

	// The type of test being performed.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ExtensionAllowsAdditionArgs")
	EFaerieContainerAddStackCheckType CheckType = EFaerieContainerAddStackCheckType::CanAddNow;

	// How do we want to add the stack.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ExtensionAllowsAdditionArgs")
	EFaerieStorageAddStackBehavior AddStackBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack;
};

/*
 * Base class for UFaerieItemContainerBase modular data types.
 * Used to store additional container state, or configure content restrictions.
 */
USTRUCT()
struct FFaerieItemContainerData
{
	GENERATED_BODY()

	virtual ~FFaerieItemContainerData() = default;

	FAERIEINVENTORY_API void PostSerialize(const FArchive& Ar);

	virtual const UScriptStruct* GetScriptStruct() const PURE_VIRTUAL(FFaerieItemContainerData::GetScriptStruct, return nullptr; )
};

template<>
struct TStructOpsTypeTraits<FFaerieItemContainerData> : public TStructOpsTypeTraitsBase2<FFaerieItemContainerData>
{
	enum
	{
		WithPostSerialize = true,
	};
};

// Macro to implement a faerie container data struct. Place in header at end of struct declaration.
#define ____FAERIE_CONTAINER_DATA_DECL(Type)\
	FAERIE_IMPL_GetScriptStruct()\
	};\
	FAERIE_IMPL_TStructOpsTypeTraits_BEGIN(Type)\
	WithPostSerialize = true,\
	FAERIE_IMPL_TStructOpsTypeTraits_END(Type)

/*
 * Base type for Container Data types that extends container functionality via virtuals.
 * Used to store additional container state, or configure content restrictions.
 */
USTRUCT()
struct FFaerieItemContainerExtensionBase : public FFaerieItemContainerData
{
	GENERATED_BODY()

	/* Called at begin play or when the extension is created during runtime. Server-only. */
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) {}

	/* Does this extension allow a stack of items, or multiple stacks, to be added to the container? */
	virtual EFaerieExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container,
		const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const { return EFaerieExtensionResponse::NoExplicitResponse; }

	/* Does this extension allow removal of an address in the container? */
	virtual EFaerieExtensionResponse AllowsRemoval(TNotNull<const UFaerieItemContainerBase*> Container,
		TNotNull<const Faerie::Container::IAddressView*> DataView, FFaerieInventoryTag Reason) const { return EFaerieExtensionResponse::NoExplicitResponse; }

	/* Does this extension allow this entry to be edited? */
	virtual EFaerieExtensionResponse AllowsEdit(TNotNull<const UFaerieItemContainerBase*> Container,
		TNotNull<const Faerie::Container::IAddressView*> DataView, FFaerieInventoryTag EditTag) const { return EFaerieExtensionResponse::NoExplicitResponse; }
};