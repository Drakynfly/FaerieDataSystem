// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemCardTags.h"
#include "UObject/Object.h"

#include "Widgets/FaerieCardBase.h"

#include "FaerieCardGenerator.generated.h"

class UFaerieItemCardToken;

using FFaerieCardGenerationResult = TDelegate<void(bool, UFaerieCardBase*)>;
DECLARE_DYNAMIC_DELEGATE_TwoParams(FFaerieCardGenerationResultDynamic, bool, Success, UFaerieCardBase*, Widget);

namespace Faerie::Card
{
	struct FSyncGeneration
	{
		APlayerController* Player;
		FFaerieItemProxy Proxy;
		FFaerieItemCardType Tag;
	};

	struct FAsyncGeneration
	{
		FAsyncGeneration(APlayerController* Player, const FFaerieItemProxy ItemProxy, const FFaerieItemCardType Tag,
						 const FFaerieCardGenerationResult& Callback)
		  : Player(Player),
			Proxy(ItemProxy),
			Tag(Tag),
			Callback(Callback) {}

		FAsyncGeneration(APlayerController* Player, const FFaerieItemProxy ItemProxy, const FFaerieItemCardType Tag,
						 const FFaerieCardGenerationResultDynamic& InCallback)
		  : Player(Player),
			Proxy(ItemProxy),
			Tag(Tag)
		{
			if (Callback.IsBound())
			{
				Callback.BindWeakLambda(Callback.GetUObject(), [InCallback](const bool Success, UFaerieCardBase* Widget)
				{
					InCallback.Execute(Success, Widget);
				});
			}
		}

		APlayerController* Player;
		FFaerieItemProxy Proxy;
		FFaerieItemCardType Tag;
		FFaerieCardGenerationResult Callback;
	};
}


/**
 *
 */
UCLASS()
class FAERIEITEMCARD_API UFaerieCardGenerator : public UObject
{
	GENERATED_BODY()

	friend class UFaerieCardSubsystem;

public:
	TSoftClassPtr<UFaerieCardBase> GetCardClassFromProxy(FFaerieItemProxy Proxy, const FFaerieItemCardType& Type) const;

	UFaerieCardBase* Generate(const Faerie::Card::FSyncGeneration& Params);
	void GenerateAsync(const Faerie::Card::FAsyncGeneration& Params);

private:
	struct FAsyncCallback
    {
    	APlayerController* Player;
    	FFaerieItemProxy Proxy;
    	TSoftClassPtr<UFaerieCardBase> CardClass;
    	FFaerieCardGenerationResult Callback;
    };

	void OnCardClassLoaded(FAsyncCallback Params);

protected:
	UPROPERTY()
	TMap<FFaerieItemCardType, TSoftClassPtr<UFaerieCardBase>> DefaultClasses;
};