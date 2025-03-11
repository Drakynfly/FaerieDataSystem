// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieCardSubsystem.h"
#include "FaerieCardGenerator.h"
#include "FaerieCardSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardSubsystem)

bool UFaerieCardSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	auto&& CardSettings = GetDefault<UFaerieCardSettings>();
	return Super::ShouldCreateSubsystem(Outer) && CardSettings->CreateCardGeneratorPlayerSubsystems;
}

void UFaerieCardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	auto&& CardSettings = GetDefault<UFaerieCardSettings>();

	Generator = NewObject<UFaerieCardGenerator>(this);
	Generator->DefaultClasses = CardSettings->FallbackClasses;
}

UFaerieCardGenerator* UFaerieCardSubsystem::GetGenerator() const
{
	return Generator;
}