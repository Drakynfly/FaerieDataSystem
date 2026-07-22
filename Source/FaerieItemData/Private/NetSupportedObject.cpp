// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "NetSupportedObject.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/NetDriver.h"
#include "GameFramework/Actor.h"

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