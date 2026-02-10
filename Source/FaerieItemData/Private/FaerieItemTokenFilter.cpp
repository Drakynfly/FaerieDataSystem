// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemTokenFilter.h"
#include "DebuggingFlags.h"
#include "FaerieHashStatics.h"
#include "FaerieItem.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemToken.h"

namespace Faerie::Token
{
	namespace Private
	{
		const TArray<TObjectPtr<UFaerieItemToken>>& FIteratorAccess::ReadTokenArray(const TNotNull<const UFaerieItem*> Item)
		{
			return Item->Tokens;
		}

		bool FIteratorAccess::StaticClassFilter(const TNotNull<const UFaerieItemToken*> Token, const TNotNull<UClass*> Class)
		{
			return Token->GetClass()->IsChildOf(Class);
		}

		bool FIteratorAccess::StaticIsMutableFilter(const TNotNull<const UFaerieItemToken*> Token)
		{
			return Token->IsMutable();
		}

		void FIteratorAccess::AddWriteLock(const TNotNull<const UFaerieItem*> Item)
		{
#if FAERIE_DEBUG
			if (Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
			{
				UE_LOG(LogFaerieItemData, Warning, TEXT("Item WriteLock++ '%u -> %u' (Faerie::Token::FIteratorAccess::AddWriteLock)"), Item->WriteLock, Item->WriteLock + 1)
			}
#endif
			Item->WriteLock++;
		}

		void FIteratorAccess::RemoveWriteLock(const TNotNull<const UFaerieItem*> Item)
		{
#if FAERIE_DEBUG
			if (Debug::CVarEnableWriteLockTracking.GetValueOnGameThread())
			{
				ensureAlways(Item->WriteLock > 0);
				UE_LOG(LogFaerieItemData, Warning, TEXT("Item WriteLock-- '%u -> %u' (Faerie::Token::FIteratorAccess::RemoveWriteLock)"), Item->WriteLock, Item->WriteLock - 1)
			}
#endif
			Item->WriteLock--;
		}

		void FIteratorAccess::HashCombineToken(const TNotNull<const UFaerieItemToken*> Token, uint32& Hash)
		{
			Hash = Hash::Combine(Hash, Token->GetTokenHash());
		}

		bool FIteratorAccess::CompareTokenMasks(const TBitArray<>& MaskA, const TNotNull<const UFaerieItem*> ItemA,
			const TBitArray<>& MaskB, const TNotNull<const UFaerieItem*> ItemB)
		{
			// This already indicates they are not equal.
			if (MaskA.Num() != MaskB.Num())
			{
				return false;
			}

			auto MaskBCopy = MaskB;

			for (TConstSetBitIterator<> ItA(MaskA); ItA; ++ItA)
			{
				const UFaerieItemToken* TokenA = ItemA->GetTokenAtIndex(ItA.GetIndex());
				if (!ensureAlways(IsValid(TokenA))) continue;

				for (TConstSetBitIterator<> ItB(MaskBCopy); ItB; ++ItB)
				{
					const UFaerieItemToken* TokenB = ItemB->GetTokenAtIndex(ItB.GetIndex());
					if (!ensureAlways(IsValid(TokenB))) continue;

					if (TokenA->CompareWith(TokenB))
					{
						// The token is a match! Remove token from B to prevent re-match, and continue to the next token in ItA.
						MaskBCopy.AccessCorrespondingBit(ItB) = false;
						break;
					}
				}

				// No match was found for a token in ItA, exit as failure.
				return false;
			}

			// All tokens in B should habe been matched. Check and exit as a success.
			check(MaskBCopy.IsEmpty())
			return true;
		}
	}
}
