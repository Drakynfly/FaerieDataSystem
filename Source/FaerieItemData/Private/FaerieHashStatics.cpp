// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieHashStatics.h"
#include "FaerieItemStackHashInstruction.h"
#include "Squirrel.h"
#include "Tokens/FaerieInfoToken.h"
#include "UObject/TextProperty.h"
#include "UObject/PropertyOptional.h"

// WARNING: Changing this will invalidate all existing hashes generated with CombineHashes.
#define COMBINING_HASHING_SEED 561333781

namespace Faerie::Hash
{
	[[nodiscard]] uint32 Combine(const uint32 A, const uint32 B)
	{
		return Squirrel::HashCombine(A, B);
	}

	FFaerieHash CombineHashes(TArray<uint32>& Hashes)
	{
		// return a 0 hash for an empty array.
		if (Hashes.IsEmpty()) return FFaerieHash();

		// Don't combine with a single hash.
		if (Hashes.Num() == 1) return FFaerieHash(Hashes[0]);

		// Sort hashes, so they are in a deterministic order.
		Algo::Sort(Hashes);

		FFaerieHash OutHash{COMBINING_HASHING_SEED};

		// Combine all hashes into the final hash
		for (const uint32 Hash : Hashes)
		{
			OutHash.Hash = Combine(Hash, OutHash.Hash);
		}

		return OutHash;
	}

	uint32 HashFProperty(const void* Ptr, const FProperty* Property)
	{
		if (Property->IsA<FBoolProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<bool>(Ptr)); // GetTypeHash is deterministic for scalars
		}
		if (Property->IsA<FByteProperty>() || Property->IsA<FEnumProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<uint8>(Ptr)); // GetTypeHash is deterministic for scalars
		}
		if (Property->IsA<FIntProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<int32>(Ptr)); // GetTypeHash is deterministic for scalars
		}
		if (Property->IsA<FInt64Property>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<int64>(Ptr)); // GetTypeHash is deterministic for scalars
		}
		if (Property->IsA<FFloatProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<float>(Ptr)); // GetTypeHash is deterministic for scalars
		}
		if (Property->IsA<FDoubleProperty>())
		{
			return GetTypeHash(*Property->ContainerPtrToValuePtr<double>(Ptr)); // GetTypeHash is deterministic for scalars
		}
		if (Property->IsA<FNameProperty>())
		{
			return TextKeyUtil::HashString(Property->ContainerPtrToValuePtr<FName>(Ptr)->ToString());
		}
		if (Property->IsA<FStrProperty>())
		{
			return TextKeyUtil::HashString(*Property->ContainerPtrToValuePtr<FString>(Ptr));
		}
		if (Property->IsA<FTextProperty>())
		{
			return TextKeyUtil::HashString(Property->ContainerPtrToValuePtr<FText>(Ptr)->BuildSourceString());
		}
		if (const FStructProperty* AsStruct = CastField<FStructProperty>(Property))
		{
			return HashStructByProps(AsStruct->ContainerPtrToValuePtr<void>(Ptr), AsStruct->Struct, true);
		}
		if (const FArrayProperty* AsArray = CastField<FArrayProperty>(Property))
		{
			unimplemented();
			return 0;
		}
		if (const FSetProperty* AsSet = CastField<FSetProperty>(Property))
		{
			unimplemented();
			return 0;
		}
		if (const FMapProperty* AsMap = CastField<FMapProperty>(Property))
		{
			unimplemented();
			return 0;
		}
		if (const FOptionalProperty* AsOptional = CastField<FOptionalProperty>(Property))
		{
			if (!AsOptional->IsSet(AsOptional->ContainerPtrToValuePtr<void>(Ptr)))
			{
				return 21294577;
			}

			return HashFProperty(Ptr, AsOptional->GetValueProperty());
		}

		unimplemented(); // What can hit this?
		return 0;
	}

	template <typename T>
	uint32 HashProps(const T* Container, const UStruct* Struct, const bool IncludeSuper)
	{
		uint32 Hash = 0;

		for (TFieldIterator<FProperty> PropIt(Struct, IncludeSuper ? EFieldIterationFlags::IncludeSuper : EFieldIterationFlags::None); PropIt; ++PropIt)
		{
			if (const FProperty* Property = *PropIt)
			{
				Hash = Combine(HashFProperty(Container, Property), Hash);
			}
		}

		return Hash;
	}

	uint32 HashStructByProps(const void* Ptr, const UScriptStruct* Struct, const bool IncludeSuper)
	{
		check(Ptr);
		check(Struct);
		return HashProps(Ptr, Struct, IncludeSuper);
	}

	uint32 HashObjectByProps(const UObject* Obj, const bool IncludeSuper)
	{
		check(Obj);
		return HashProps(Obj, Obj->GetClass(), IncludeSuper);
	}

	FFaerieHash HashItemSet(const TSet<const UFaerieItem*>& Items,
							const FItemHashFunction& Function)
	{
		TArray<uint32> Hashes;

		for (const UFaerieItem* Item : Items)
		{
			Hashes.Add(Function(Item));
		}

		return CombineHashes(Hashes);
	}

	uint32 HashItemByName(const UFaerieItem* Item)
	{
		if (!IsValid(Item)) return 0;

		if (const UFaerieInfoToken* InfoToken = Item->GetToken<UFaerieInfoToken>())
		{
			return TextKeyUtil::HashString(InfoToken->GetItemName().BuildSourceString());
		}

		return 0;
	}
}