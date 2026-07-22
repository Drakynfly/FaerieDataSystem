// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BlueprintClassUtils.h"
#include "FaerieDataUtilsLog.h"

#include "Components/SceneComponent.h"

#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlueprintClassUtils)

UPackage* UBlueprintClassUtils::GetPackage(const UObject* Object)
{
	if (!IsValid(Object)) return nullptr;
	return Object->GetPackage();
}

UObject* UBlueprintClassUtils::GetTypedOuter(const UObject* Object, const TSubclassOf<UObject> Class)
{
	if (!IsValid(Object)) return nullptr;
	return Object->GetTypedOuter(Class);
}

USceneComponent* UBlueprintClassUtils::GetTypedParent(const USceneComponent* Component, const TSubclassOf<USceneComponent> Class)
{
	if (!IsValid(Component)) return nullptr;

	USceneComponent* Result = nullptr;
	for (USceneComponent* NextOuter = Component->GetAttachParent(); Result == nullptr && NextOuter != nullptr; NextOuter = NextOuter->GetAttachParent() )
	{
		if (NextOuter->IsA(Class))
		{
			Result = NextOuter;
		}
	}
	return Result;
}

bool UBlueprintClassUtils::AddReplicatedSubObject(AActor* Actor, UObject* Object)
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

bool UBlueprintClassUtils::RemoveReplicatedSubObject(AActor* Actor, UObject* Object)
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
