﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieCardGenerateAsync.h"
#include "FaerieCardGenerator.h"
#include "FaerieCardGeneratorInterface.h"
#include "FaerieItemCardModule.h"
#include "FaerieItemDataProxy.h"
#include "CardTokens/CustomCardClass.h"

bool UFaerieCardGenerateAsync::GenerateItemCard(APlayerController* OwningPlayer,
												const TScriptInterface<IFaerieCardGeneratorInterface> Generator,
												const FFaerieItemProxy Proxy, const TSubclassOf<UCustomCardClass> Type,
												UFaerieCardBase*& Widget)
{
	if (Generator.GetInterface() == nullptr) return false;

	auto&& GeneratorImpl = Generator->GetGenerator();
	if (!IsValid(GeneratorImpl))
	{
		UE_LOG(LogFaerieItemCard, Error, TEXT("Failed to get Generator from interface!"))
		return false;
	}

	Widget = GeneratorImpl->Generate(Faerie::Card::FSyncGeneration(OwningPlayer, Proxy, Type));
	return IsValid(Widget);
}

UFaerieCardGenerateAsync* UFaerieCardGenerateAsync::GenerateItemCardAsync(APlayerController* OwningPlayer,
	const TScriptInterface<IFaerieCardGeneratorInterface> Generator, const FFaerieItemProxy Proxy,
	const TSubclassOf<UCustomCardClass> Type)
{
	if (!IsValid(Generator.GetObject()))
	{
		return nullptr;
	}

	UFaerieCardGenerateAsync* AsyncAction = NewObject<UFaerieCardGenerateAsync>();
	AsyncAction->Generator = Generator->GetGenerator();
	if (!IsValid(AsyncAction->Generator))
	{
		UE_LOG(LogFaerieItemCard, Error, TEXT("Failed to get Generator from interface!"))
		return nullptr;
	}

	AsyncAction->OwningPlayer = OwningPlayer;
	AsyncAction->Proxy = Proxy;
	AsyncAction->Class = Type;

	return AsyncAction;
}

void UFaerieCardGenerateAsync::Activate()
{
	Generator->GenerateAsync(Faerie::Card::FAsyncGeneration(OwningPlayer, Proxy, Class, FFaerieCardGenerationResult::CreateUObject(this, &ThisClass::OnCardGenerationFinished)));
}

void UFaerieCardGenerateAsync::OnCardGenerationFinished(const bool Success, UFaerieCardBase* Widget)
{
	if (Success)
	{
		OnSuccess.Broadcast(Widget);
	}
	else
	{
		OnFailure.Broadcast();
	}
}