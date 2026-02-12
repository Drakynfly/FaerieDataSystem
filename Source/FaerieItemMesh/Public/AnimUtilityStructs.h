// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "AnimUtilityStructs.generated.h"

USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FSocketAttachment
{
	GENERATED_BODY()

	/** Attach to this bone */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attachment Data")
	FName Socket;

	/** Attach with this offset */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Attachment Data")
	FTransform Offset;
};

/*
 * A skeletal mesh and paired animation blueprint class and/or animation asset.
*/
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FSkeletonAndAnimation
{
	GENERATED_BODY()

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SkeletonAndAnimation")
	TObjectPtr<const USkeletalMesh> Mesh = nullptr;

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SkeletonAndAnimation")
	TSubclassOf<UAnimInstance> AnimClass = nullptr;

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SkeletonAndAnimation")
	TObjectPtr<UAnimationAsset> AnimationAsset = nullptr;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FSkeletonAndAnimation& Other) const
	{
		return Mesh == Other.Mesh
			&& AnimClass == Other.AnimClass
			&& AnimationAsset == Other.AnimationAsset;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FSkeletonAndAnimation& Thing)
	{
		return FCrc::MemCrc32(&Thing, sizeof(FSkeletonAndAnimation));
	}
};

/*
 * A skeletal mesh and paired animation blueprint class and/or animation asset.
*/
USTRUCT(BlueprintType)
struct FAERIEITEMMESH_API FSoftSkeletonAndAnimation
{
	GENERATED_BODY()

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SoftSkeletonAndAnimation")
	TSoftObjectPtr<USkeletalMesh> Mesh = nullptr;

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SoftSkeletonAndAnimation")
	TSoftClassPtr<UAnimInstance> AnimClass = nullptr;

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SoftSkeletonAndAnimation")
	TSoftObjectPtr<UAnimationAsset> AnimationAsset = nullptr;

	FSkeletonAndAnimation LoadSynchronous() const;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FSoftSkeletonAndAnimation& Other) const
	{
		return Mesh == Other.Mesh
			&& AnimClass == Other.AnimClass
			&& AnimationAsset == Other.AnimationAsset;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FSoftSkeletonAndAnimation& Thing)
	{
		return FCrc::MemCrc32(&Thing, sizeof(FSoftSkeletonAndAnimation));
	}
};