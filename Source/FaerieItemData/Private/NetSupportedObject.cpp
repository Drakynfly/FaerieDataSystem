// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "NetSupportedObject.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(NetSupportedObject)

bool UNetSupportedObject::IsSupportedForNetworking() const
{
	return true;
}

void UNetSupportedObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Add any Blueprint properties
	if (const UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

#if UE_WITH_IRIS
void UNetSupportedObject::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context,
													   const UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	// Build descriptors and allocate PropertyReplicationFragments for this object
	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif

UWorld* UActorSubobjectBase::GetWorld() const
{
	if (const AActor* Owner = GetOwner())
	{
		return Owner->GetWorld();
	}

	return nullptr;
}

int32 UActorSubobjectBase::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter() != nullptr);
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UActorSubobjectBase::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));

	// Call "Remote" (aka, RPC) functions through the actors NetDriver

	if (AActor* Owner = GetOwner();
		IsValid(Owner))
	{
		if (UNetDriver* NetDriver = Owner->GetNetDriver())
		{
			NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
			return true;
		}
	}

	return false;
}

void UActorSubobjectBase::PostInitProperties()
{
	Super::PostInitProperties();

	if (!IsTemplate())
	{
		// Objects of this class should always be outer'ed to an Actor.
		check(GetOwner());
	}
}

AActor* UActorSubobjectBase::GetOwner() const
{
	return GetTypedOuter<AActor>();
}

AActor* UActorSubobjectBase::GetOwnerChecked() const
{
	AActor* Owner = GetOwner();
	check(Owner);
	return Owner;
}