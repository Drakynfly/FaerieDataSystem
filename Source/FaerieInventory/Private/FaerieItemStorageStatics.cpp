// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageStatics.h"
#include "FaerieInventorySettings.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemToken.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Tokens/FaerieItemStorageToken.h"

namespace Faerie
{
	bool ValidateItemData(const UFaerieItem* Item)
	{
		if (!ensure(IsValid(Item)))
		{
			UE_LOG(LogTemp, Error, TEXT("ValidateItemData: Item pointer is invalid."))
			return false;
		}

		bool HitError = false;

#if WITH_EDITOR
		static TSet<const UFaerieItem*> RecursiveValidationTracker;
		RecursiveValidationTracker.Add(Item);
#endif

		auto Tokens = Item->GetTokens();
		for (int32 i = 0; i < Tokens.Num(); ++i)
		{
			UFaerieItemToken* Token = Tokens[i];

			if (!IsValid(Token))
			{
				UE_LOG(LogTemp, Error, TEXT("ValidateItemData: Token[%i] is invalid."), i)
				HitError |= true;
			}

			if (auto ExtInterface = Cast<IFaerieContainerExtensionInterface>(Token))
			{
				ExtInterface->GetExtensionGroup()->ValidateGroup();
			}

			if (UFaerieItemContainerToken* SubStorage = Cast<UFaerieItemContainerToken>(Token))
			{
				SubStorage->GetItemContainer()->ForEachItem(
					[&HitError](const UFaerieItem* SubItem)
					{
#if WITH_EDITOR
						if (RecursiveValidationTracker.Contains(SubItem))
						{
							UE_LOG(LogTemp, Error, TEXT("ValidateItemData: Item '%s' is somehow contained in itself :/"), *SubItem->GetName())
							HitError |= true;
							return;
						}
#endif
						HitError |= !ValidateItemData(SubItem);
					});
			}
		}

#if WITH_EDITOR
		RecursiveValidationTracker.Remove(Item);
#endif

		return !HitError;
	}

	void ReleaseOwnership_Impl(UObject* Owner, const UFaerieItem* Item, const bool IsSubItem)
	{
		if (!ensure(IsValid(Item))) return;

		// When Items are potentially mutable, undo any modifications that rely on this owner.
		if (UFaerieItem* MutableItem = Item->MutateCast())
		{
			// @todo this logic could be moved to UFaerieItem::PostRename (if we enforce the RenameBehavior)
			AActor* Actor = Owner->GetTypedOuter<AActor>();
			const bool RegisteredWithActor = IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList();
			if (RegisteredWithActor)
			{
				MutableItem->DeinitializeNetObject(Actor);
				Actor->RemoveReplicatedSubObject(MutableItem);
			}
			MutableItem->ForEachToken(
				[&](const TObjectPtr<UFaerieItemToken>& Token)
				{
					if (!Token->IsMutable()) return Continue;

					if (RegisteredWithActor)
					{
						Token->DeinitializeNetObject(Actor);
						Actor->RemoveReplicatedSubObject(Token);
					}

					// If the token has an extension group, clear its parent.
					if (IFaerieContainerExtensionInterface* TokenWithExtension = Cast<IFaerieContainerExtensionInterface>(Token))
					{
						TokenWithExtension->GetExtensionGroup()->SetParentGroup(nullptr);
					}

					// If the token contains nested items, release ownership recursively.
					if (UFaerieItemContainerToken* ContainerToken = Cast<UFaerieItemContainerToken>(Token))
					{
						ContainerToken->GetItemContainer()->ForEachItem([Owner](const UFaerieItem* ChildItem)
						{
							ReleaseOwnership_Impl(Owner, ChildItem, true);
						});
					}

					return Continue;
				});

			if (!IsSubItem)
			{
				// If we renamed the item to ourself when we took ownership of this item, then we need to release that now.
				if (MutableItem->GetOuter() == Owner)
				{
					MutableItem->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors);
				}

				// Unbind from the mutation hook.
				MutableItem->GetNotifyOwnerOfSelfMutation().Unbind();
			}
		}
	}

	void ClearOwnership(const UFaerieItem* Item)
	{
		if (!ensure(IsValid(Item))) return;

		if (UObject* Outer = Item->GetOuter())
		{
			if (Outer != GetTransientPackage())
			{
				ReleaseOwnership(Outer, Item);
			}
		}
	}

	void TakeOwnership_Impl(UObject* Owner, const UFaerieItem* Item, const bool IsSubItem)
	{
		if (!ensure(IsValid(Item))) return;

		if (UFaerieItem* MutableItem = Item->MutateCast())
		{
			checkfSlow(Item->GetOuter() != Owner, TEXT("TakeOwnership called on item we already own. Incorrect recursion likely the cause!"));

			// Children of items that have already been moved to this owner will also own us, so skip the transient check, but perform the rest of this function.
			if (!Item->IsInOuter(Owner))
			{
				checkfSlow(Item->GetOuter() == GetTransientPackage(), TEXT("ReleaseOwnership was not called correctly on this item, before attempting to give ownership here!"));
			}

			//UE_LOG(LogTemp, Verbose, TEXT("Assigning Ownership of %s to %s"), *Item->GetFullName(), *Owner->GetName())

			// Children do not get renamed. They already belong to this outer chain if we are renamed.
			// We also don't bind to the mutation hook if we are a subitem.
			if (!IsSubItem)
			{
				if (GetDefault<UFaerieInventorySettings>()->ContainerMutableBehavior == EFaerieContainerOwnershipBehavior::Rename)
				{
					MutableItem->Rename(nullptr, Owner, REN_DontCreateRedirectors);
				}

				checkfSlow(!MutableItem->GetNotifyOwnerOfSelfMutation().IsBound(), TEXT("This should always have been unbound by the previous owner!"))

				if (IFaerieItemOwnerInterface* OwnerInterface = Cast<IFaerieItemOwnerInterface>(Owner))
				{
					MutableItem->GetNotifyOwnerOfSelfMutation().BindRaw(OwnerInterface, &IFaerieItemOwnerInterface::OnItemMutated);
				}
			}

			// @todo this logic could be moved to UFaerieItem::PostRename (if we enforce the RenameBehavior)
			AActor* Actor = Owner->GetTypedOuter<AActor>();
			const bool RegisterWithActor = IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList();
			if (RegisterWithActor)
			{
				Actor->AddReplicatedSubObject(MutableItem);
				MutableItem->InitializeNetObject(Actor);
			}

			UItemContainerExtensionGroup* OuterExtensions = nullptr;
			if (IFaerieContainerExtensionInterface* OuterWithExtension = Cast<IFaerieContainerExtensionInterface>(Owner))
			{
				OuterExtensions = OuterWithExtension->GetExtensionGroup();
			}

			MutableItem->ForEachToken(
				[&](const TObjectPtr<UFaerieItemToken>& Token)
				{
					if (!Token->IsMutable()) return Continue;

					if (RegisterWithActor)
					{
						Actor->AddReplicatedSubObject(Token);
						Token->InitializeNetObject(Actor);
					}

					// If the token has an extension group, set its parent to ours.
					if (IsValid(OuterExtensions))
					{
						if (IFaerieContainerExtensionInterface* TokenWithExtension = Cast<IFaerieContainerExtensionInterface>(Token))
						{
							TokenWithExtension->GetExtensionGroup()->SetParentGroup(OuterExtensions);
						}
					}

					// If the token contains nested items, take ownership recursively.
					if (UFaerieItemContainerToken* ContainerToken = Cast<UFaerieItemContainerToken>(Token))
					{
						ContainerToken->GetItemContainer()->ForEachItem([Owner](const UFaerieItem* ChildItem)
						{
							TakeOwnership_Impl(Owner, ChildItem, true);
						});
					}

					return Continue;
				});
		}
	}

	void ReleaseOwnership(UObject* Owner, const UFaerieItem* Item)
	{
		ReleaseOwnership_Impl(Owner, Item, false);
	}

	void TakeOwnership(UObject* Owner, const UFaerieItem* Item)
	{
		TakeOwnership_Impl(Owner, Item, false);
	}
}
