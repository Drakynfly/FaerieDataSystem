// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieCardGenerateAsync.h"
#include "FaerieCardGenerator.h"
#include "FaerieCardGeneratorInterface.h"
#include "FaerieItemCardLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardGenerateAsync)

bool UFaerieCardGenerateAsync::GenerateItemCard(APlayerController* OwningPlayer,
												const TScriptInterface<IFaerieCardGeneratorInterface> Generator,
												const FFaerieItemProxy Proxy, const FFaerieItemCardType Tag,
												UFaerieCardBase*& Widget)
{
	if (Generator.GetInterface() == nullptr) return false;

	auto&& GeneratorImpl = Generator->GetGenerator();
	if (!IsValid(GeneratorImpl))
	{
		UE_LOG(LogFaerieItemCard, Error, TEXT("Failed to get Generator from interface!"))
		return false;
	}

	Widget = GeneratorImpl->Generate(Faerie::Card::FSyncGeneration(OwningPlayer, Proxy, Tag));
	return IsValid(Widget);
}

UFaerieCardGenerateAsync* UFaerieCardGenerateAsync::GenerateItemCardAsync(APlayerController* OwningPlayer,
	const TScriptInterface<IFaerieCardGeneratorInterface> Generator, const FFaerieItemProxy Proxy,
	const FFaerieItemCardType Tag)
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
	AsyncAction->Tag = Tag;

	return AsyncAction;
}

void UFaerieCardGenerateAsync::Activate()
{
	Generator->GenerateAsync(Faerie::Card::FAsyncGeneration(OwningPlayer, Proxy, Tag, FFaerieCardGenerationResult::CreateUObject(this, &ThisClass::OnCardGenerationFinished)));
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