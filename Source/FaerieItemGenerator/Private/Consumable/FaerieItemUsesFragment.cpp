// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/FaerieItemUsesFragment.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemSource.h"
#include "FaerieUnownedItemStack.h"
#include "ItemInstancingContext_Crafting.h"
#include "MassCommandBuffer.h"
#include "MassEntityManager.h"

#include "MassReplication/FaerieViewModelSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUsesFragment)

void FFaerieItemLastUseLogicBase::HandleOnLastUse(const FFaerieItemLastUseLogicBase* ThisBase, FMassEntityManager& EntityManager,
	const FFaerieItemProxy& Proxy, const bool ProcessAsync) const
{
	check(OnLastUseFuncPtr)
	OnLastUseFuncPtr(ThisBase, EntityManager, Proxy, ProcessAsync);
}

FFaerieItemLastUseLogic_Destroy::FFaerieItemLastUseLogic_Destroy()
{
	OnLastUseFuncPtr = &OnLastUse_Destroy;
}

void FFaerieItemLastUseLogic_Destroy::OnLastUse_Destroy(const FFaerieItemLastUseLogicBase* ThisBase, FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, bool ProcessAsync)
{
	if (UFaerieItemContainerBase* Container = Cast<UFaerieItemContainerBase>(Proxy.GetItemOwner()))
	{
		// Destroy and cast into the aether.
		Container->DestroyStack(Proxy, 1);
	}
}

FFaerieItemLastUseLogic_Replace::FFaerieItemLastUseLogic_Replace()
{
	OnLastUseFuncPtr = &OnLastUse_Replace;
}

void FFaerieItemLastUseLogic_Replace::OnLastUse_Replace(const FFaerieItemLastUseLogicBase* ThisBase, FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const bool ProcessAsync)
{
	const FFaerieItemLastUseLogic_Replace* ThisPtr = static_cast<const FFaerieItemLastUseLogic_Replace*>(ThisBase);

	TSoftObjectPtr<const UObject> BaseItemSourceObject = ThisPtr->BaseItemSource.Object;
	if (BaseItemSourceObject.IsNull()) return;

	struct FLocal
	{
		static void GameThread_Run(const FFaerieItemProxy& InProxy, FMassEntityManager& InEntityManager, const UObject* Object)
		{
			const IFaerieItemSource* ItemSource = Cast<IFaerieItemSource>(Object);
			if (!ItemSource)
			{
				UE_LOG(LogItemGeneration, Error, TEXT("Invalid ItemSource for FFaerieItemLastUseLogic_Replace on '%s'"), *InProxy.GetProxyObject()->GetName())
				return;
			}

			UFaerieItemContainerBase* Container = Cast<UFaerieItemContainerBase>(InProxy.GetItemOwner());
			if (!IsValid(Container))
			{
				UE_LOG(LogItemGeneration, Error, TEXT("Invalid Container (no item instance owner) for FFaerieItemLastUseLogic_Replace on '%s'"), *InProxy.GetProxyObject()->GetName())
				return;
			}

			FFaerieItemInstancingContext Context;
			Context.EntityManager = &InEntityManager;
			const Faerie::ItemData::FGetInstanceResult Result = ItemSource->CreateItemStack(Context);
			if (!Result.IsValid())
			{
				UE_LOG(LogItemGeneration, Error, TEXT("Failed to create item instance for FFaerieItemLastUseLogic_Replace on '%s'"), *InProxy.GetProxyObject()->GetName())
				return;
			}

			// Destroy and cast into the aether.
			Container->DestroyStack(InProxy);

			// Possess new item
			if (!Container->Possess(Result.WithInitialization()))
			{
				UE_LOG(LogItemGeneration, Error, TEXT("Container failed to possess new item instance for FFaerieItemLastUseLogic_Replace on '%s'"), *InProxy.GetProxyObject()->GetName())
			}
		}
	};

	if (BaseItemSourceObject.IsPending())
	{
		if (ProcessAsync && Proxy.IsSafeToPersist())
		{
			BaseItemSourceObject.LoadAsync(FLoadSoftObjectPathAsyncDelegate::CreateWeakLambda(Proxy.GetProxyObject(),
				[Proxy, &EntityManager](const FSoftObjectPath& Path, const UObject* Object)
				{
					FLocal::GameThread_Run(Proxy, EntityManager, Object);
				}));
			return;
		}
	}

	// Note: Known LoadSync code path; accepted use.
	FLocal::GameThread_Run(Proxy, EntityManager, BaseItemSourceObject.LoadSynchronous());
}

namespace Faerie::ItemData
{
	FAERIE_REGISTER_TRAITS(FFaerieItemUses)

	namespace
	{
		const FName FieldNames[2]
		{
			GET_MEMBER_NAME_CHECKED(FFaerieItemUses, UsesRemaining),
			GET_MEMBER_NAME_CHECKED(FFaerieItemUses, MaxUses)
		};

		const FFieldChange& GetUsesRemainingFieldData()
		{
			static const FFieldChange UsesRemainingFieldData(FFaerieItemUses::StaticStruct(), MakeConstArrayView(FieldNames, 1));
			return UsesRemainingFieldData;
		}

		const FFieldChange& GetMaxUsesFieldData()
		{
			static const FFieldChange MaxUsesFieldData(FFaerieItemUses::StaticStruct(), MakeConstArrayView(FieldNames+1, 1));
			return MaxUsesFieldData;
		}

		const FFieldChange& GetAllItemUsesFieldData()
		{
			static const FFieldChange AllFieldData(FFaerieItemUses::StaticStruct(), MakeConstArrayView(FieldNames, 2));
			return AllFieldData;
		}
	}

	FUsesHelper::FUsesHelper(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Instance)
	  : EntityManager(&EntityManager), Item(Instance)
	{
		// Look for a live fragment if we have an entity manager
		if (const FFaerieItemUses* CapacityFragment = GetEntityFragment<FFaerieItemUses>(EntityManager, Item.GetMassEntityHandle()))
		{
			FragmentPtr = CapacityFragment;
		}

		// For the default value.
		if (const FFaerieItemUses* DefaultCapacityFragment = Item.GetItemPtr()->GetDefaultFragment<FFaerieItemUses>())
		{
			Defaults_FragmentPtr = DefaultCapacityFragment;
		}
	}

	void FUsesHelper::CreateFragment(FMassEntityManager& InEntityManager, FFaerieItemInstance& Instance, const TOptional<int32>& MaxUses, const TOptional<int32>& InitialUses)
	{
		checkfSlow(!FragmentPtr, TEXT("CreateFragment should not be called for an item that already has a uses fragment."))
		checkfSlow(EntityManager, TEXT("An entity manager is required to initialize mass fragments"))

		FFaerieItemUses NewUses;
		if (MaxUses.IsSet())
		{
			NewUses.MaxUses = MaxUses.GetValue();
		}
		else if (Defaults_FragmentPtr)
		{
			NewUses.MaxUses = Defaults_FragmentPtr->MaxUses;
		}
		else
		{
			NewUses.MaxUses = 1;
		}
		if (InitialUses.IsSet())
		{
			NewUses.UsesRemaining = InitialUses.GetValue();
		}
		else if (Defaults_FragmentPtr)
		{
			NewUses.UsesRemaining = Defaults_FragmentPtr->UsesRemaining;
		}
		else
		{
			NewUses.UsesRemaining = NewUses.MaxUses;
		}

		FInstancedStruct Fragment;
		Fragment.InitializeAs<FFaerieItemUses>(NewUses);
		Instance.AddFragment(InEntityManager, MoveTemp(Fragment));
		if (const FFaerieItemUses* UsesStruct = GetEntityFragment<FFaerieItemUses>(*EntityManager, Item.GetMassEntityHandle()))
		{
			FragmentPtr = UsesStruct;
		}
	}

	bool FUsesHelper::HasUsesRemaining(const int32 Amount) const
	{
		return GetFragmentValue()->UsesRemaining >= Amount;
	}

	void FUsesHelper::AddUses(const int32 Amount, const bool ClampToMax)
	{
		if (Amount <= 0) return;

		// Ensure live fragment is registered.
		check(HasRuntimeFragment())

		EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
			[Instance = Item, Amount, ClampToMax](FMassEntityManager& InEntityManager)
			{
				const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
				if (InEntityManager.IsEntityValid(Entity))
				{
					return;
				}

				// Get fragment
				auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemUses>(Entity);

				// Assign new value
				if (ClampToMax)
				{
					const int32 NewUses = FMath::Min(Fragment.UsesRemaining + Amount, Fragment.MaxUses);
					Fragment.UsesRemaining = NewUses;
				}
				else
				{
					Fragment.UsesRemaining += Amount;
				}

				// Broadcast change and tell replication to pass this along to clients.
				const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
				Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetUsesRemainingFieldData());
			});
	}

	void FUsesHelper::RemoveUses(const FFaerieItemProxy& Proxy_TempForNow, const int32 Amount)
	{
		if (Amount <= 0) return;

		// Ensure live fragment is registered.
		check(HasRuntimeFragment())
		check(Proxy_TempForNow.IsSafeToPersist())

		EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
			[Proxy_TempForNow, Amount](FMassEntityManager& InEntityManager)
			{
				if (!Proxy_TempForNow.IsValid()) return;

				// Get fragment
				FFaerieItemInstance Instance(Proxy_TempForNow.GetItemInstance().GetValue());
				const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
				auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemUses>(Entity);

				// Assign new value
				const int32 NewUses = FMath::Max(Fragment.UsesRemaining - Amount, 0);
				Fragment.UsesRemaining = NewUses;

				// Broadcast change and tell replication to pass this along to clients.
				const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
				Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetUsesRemainingFieldData());

				if (Fragment.UsesRemaining <= 0)
				{
					auto LastUseLogicFragment = GetEntityFragmentOrDefault<FFaerieItemLastUseLogicBase>(&InEntityManager, Instance);
					if (LastUseLogicFragment.IsValid())
					{
						LastUseLogicFragment->HandleOnLastUse(LastUseLogicFragment.GetPtr<FFaerieItemLastUseLogicBase>(), InEntityManager, Proxy_TempForNow, true);
					}
				}
			});
	}

	void FUsesHelper::SetUses(int32 Amount)
	{
		// Ensure live fragment is registered.
		check(HasRuntimeFragment())

		// Always clamp Amount to a positive value
		Amount = FMath::Max(Amount, 0);

		if (FragmentPtr->UsesRemaining == Amount)
		{
			// Already at value, nothing to change.
			return;
		}

		EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
			[Instance = Item, Amount](FMassEntityManager& InEntityManager)
			{
				const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
				if (InEntityManager.IsEntityValid(Entity))
				{
					return;
				}

				// Get fragment
				auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemUses>(Entity);

				// Assign new value
				Fragment.UsesRemaining = Amount;

				// Broadcast change and tell replication to pass this along to clients.
				const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
				Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetUsesRemainingFieldData());
			});
	}

	void FUsesHelper::ResetUses()
	{
		if (!ensureMsgf(HasDefaultFragment(), TEXT("ResetUses should not be called for an instance that does not have defaults!")))
		{
			return;
		}

		// Ensure live fragment is registered.
		check(HasRuntimeFragment())

		if (FragmentPtr->UsesRemaining == Defaults_FragmentPtr->UsesRemaining)
		{
			// Already at default, nothing to reset.
			return;
		}

		EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
			[Instance = Item, NewValue = Defaults_FragmentPtr->UsesRemaining](FMassEntityManager& InEntityManager)
			{
				const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
				if (InEntityManager.IsEntityValid(Entity))
				{
					return;
				}

				// Get fragment
				auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemUses>(Entity);

				// Assign new value
				Fragment.UsesRemaining = NewValue;

				// Broadcast change and tell replication to pass this along to clients.
				const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
				Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetUsesRemainingFieldData());
			});
	}

	void FUsesHelper::SetMaxUses(int32 Value, bool ClampRemainingIfOverMax)
	{
		// Ensure live fragment is registered.
		check(HasRuntimeFragment())

		// Always clamp Value to a positive value
		Value = FMath::Max(Value, 0);

		if (FragmentPtr->MaxUses == Value)
		{
			// Already at value, nothing to change.
			return;
		}

		EntityManager->Defer().PushCommand<FMassDeferredSetCommand>(
			[Instance = Item, Value, ClampRemainingIfOverMax](FMassEntityManager& InEntityManager)
			{
				const FMassEntityHandle Entity = Instance.GetMassEntityHandle();
				if (InEntityManager.IsEntityValid(Entity))
				{
					return;
				}

				// Get fragment
				auto& Fragment = InEntityManager.GetFragmentDataChecked<FFaerieItemUses>(Entity);

				// Assign new value
				Fragment.MaxUses = Value;
				bool ChangedRemaining = false;
				if (ClampRemainingIfOverMax && (Fragment.MaxUses > Fragment.UsesRemaining))
				{
					Fragment.UsesRemaining = Fragment.MaxUses;
					ChangedRemaining = true;
				}

				// Broadcast change and tell replication to pass this along to clients.
				const TConstStructView<FFaerieMassFragment> FragmentView = Fragment;
				if (ChangedRemaining)
				{
					Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetAllItemUsesFieldData());
				}
				else
				{
					Instance.OnItemFragmentEdited(InEntityManager, FragmentView, GetMaxUsesFieldData());
				}
			});
	}
}
