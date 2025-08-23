// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BlueprintReplicationUtils.h"
#include "FaerieDataUtilsLog.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlueprintReplicationUtils)

bool UBlueprintReplicationUtils::AddReplicatedSubObject(AActor* Actor, UObject* Object)
{
	if (Object && Actor)
	{
		if (Object->GetTypedOuter<AActor>() != Actor)
		{
			UE_LOG(LogFaerieDataUtils, Warning,
				TEXT("AddReplicatedSubObject: Should not register Object to Actor that does not own it."
						" GivenActor: '%s', Object: '%s' DirectOuter: '%s', FirstActorOuter: '%s'"),
						*Actor->GetName(), *Object->GetName(), IsValid(Object->GetOuter()) ? *Object->GetOuter()->GetName() : TEXT("none"),
						IsValid(Object->GetTypedOuter<AActor>()) ? *Object->GetTypedOuter<AActor>()->GetName() : TEXT("none"))
			return false;
		}

		Actor->AddReplicatedSubObject(Object);
		return Actor->IsReplicatedSubObjectRegistered(Object);
	}
	return false;
}

bool UBlueprintReplicationUtils::RemoveReplicatedSubObject(AActor* Actor, UObject* Object)
{
	if (Object && Actor)
	{
		if (Object->GetTypedOuter<AActor>() != Actor)
		{
			UE_LOG(LogFaerieDataUtils, Warning,
				TEXT("RemoveReplicatedSubObject: Should not remove Object from Actor that does not own it."
						" GivenActor: '%s', Object: '%s' DirectOuter: '%s', FirstActorOuter: '%s'"),
						*Actor->GetName(), *Object->GetName(), IsValid(Object->GetOuter()) ? *Object->GetOuter()->GetName() : TEXT("none"),
						IsValid(Object->GetTypedOuter<AActor>()) ? *Object->GetTypedOuter<AActor>()->GetName() : TEXT("none"))
			return false;
		}

		Actor->RemoveReplicatedSubObject(Object);
		return true;
	}
	return false;
}
