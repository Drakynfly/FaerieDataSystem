// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BlueprintClassUtils.h"
#include "Components/SceneComponent.h"

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