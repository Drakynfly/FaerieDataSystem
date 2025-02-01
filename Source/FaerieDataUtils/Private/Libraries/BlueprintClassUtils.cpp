﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BlueprintClassUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlueprintClassUtils)

UObject* UBlueprintClassUtils::GetTypedOuter(UObject* Object, const TSubclassOf<UObject> Class)
{
	if (!IsValid(Object)) return nullptr;
	return Object->GetTypedOuter(Class);
}

USceneComponent* UBlueprintClassUtils::GetTypedParent(USceneComponent* Component, const TSubclassOf<USceneComponent> Class)
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