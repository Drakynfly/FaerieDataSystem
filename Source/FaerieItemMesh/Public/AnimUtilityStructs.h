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
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SkeletonAndAnimation")
	TSubclassOf<UAnimInstance> AnimClass = nullptr;

	UPROPERTY(NoClear, BlueprintReadWrite, EditAnywhere, Category = "SkeletonAndAnimation")
	TObjectPtr<UAnimationAsset> AnimationAsset = nullptr;

	friend bool operator==(const FSkeletonAndAnimation& Lhs, const FSkeletonAndAnimation& Rhs)
	{
		return Lhs.Mesh == Rhs.Mesh
			&& Lhs.AnimClass == Rhs.AnimClass
			&& Lhs.AnimationAsset == Rhs.AnimationAsset;
	}

	friend bool operator!=(const FSkeletonAndAnimation& Lhs, const FSkeletonAndAnimation& Rhs)
	{
		return !(Lhs == Rhs);
	}

	FORCEINLINE friend uint32 GetTypeHash(const FSkeletonAndAnimation& Thing)
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

	friend bool operator==(const FSoftSkeletonAndAnimation& Lhs, const FSoftSkeletonAndAnimation& Rhs)
	{
		return Lhs.Mesh == Rhs.Mesh
			&& Lhs.AnimClass == Rhs.AnimClass
			&& Lhs.AnimationAsset == Rhs.AnimationAsset;
	}

	friend bool operator!=(const FSoftSkeletonAndAnimation& Lhs, const FSoftSkeletonAndAnimation& Rhs)
	{
		return !(Lhs == Rhs);
	}

	FSkeletonAndAnimation LoadSynchronous() const;

	FORCEINLINE friend uint32 GetTypeHash(const FSoftSkeletonAndAnimation& Thing)
	{
		return FCrc::MemCrc32(&Thing, sizeof(FSoftSkeletonAndAnimation));
	}
};