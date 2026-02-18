// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieGridEnums.h"
#include "SpatialTypes.generated.h"

namespace Faerie::Extensions
{
	class FBitMatrix;
}

/*
 * A shape composed of 2D points.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORYCONTENT_API FFaerieGridShape
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FaerieGridShape")
	TArray<FIntPoint> Points;

	// Make a rectangular shape
	static FFaerieGridShape MakeSquare(int32 Size);
	static FFaerieGridShape MakeRect(int32 Height, int32 Width);

	bool IsValid() const;
	FIntPoint GetSize() const;
	FIntRect GetBounds() const;
	FIntPoint GetShapeCenter() const;
	FIntPoint GetIndexedShapeCenter() const;
	FIntPoint GetShapeAverageCenter() const;
	bool IsSymmetrical() const;

	//Matrix
	Faerie::Extensions::FBitMatrix ToMatrix() const;
	static TArray<FIntPoint> MatrixToPoints(const Faerie::Extensions::FBitMatrix& Matrix, FIntPoint Origin);
	static void RotateMatrixClockwise(Faerie::Extensions::FBitMatrix& Matrix, ESpatialItemRotation Rotation = ESpatialItemRotation::None);

	bool Contains(const FIntPoint& Position) const;
	[[nodiscard]] bool Overlaps(const FFaerieGridShape& Other) const;

	void TranslateInline(const FIntPoint& Position);
	[[nodiscard]] FFaerieGridShape Translate(const FIntPoint& Position) const;

	void RotateInline(ESpatialItemRotation Rotation, const bool Reset = false);
	[[nodiscard]] FFaerieGridShape Rotate(ESpatialItemRotation Rotation, const bool Reset = false) const;

	void RotateAroundInline_90(const FIntPoint& PivotPoint);
	[[nodiscard]] FFaerieGridShape RotateAround_90(const FIntPoint& PivotPoint) const;
	void RotateAroundInline_180(const FIntPoint& PivotPoint);
	[[nodiscard]] FFaerieGridShape RotateAround_180(const FIntPoint& PivotPoint) const;
	void RotateAroundInline_270(const FIntPoint& PivotPoint);
	[[nodiscard]] FFaerieGridShape RotateAround_270(const FIntPoint& PivotPoint) const;

	void RotateAroundCenterInline();
	[[nodiscard]] FFaerieGridShape RotateAroundCenter() const;

	void NormalizeInline();
	[[nodiscard]] FFaerieGridShape Normalize() const;

	[[nodiscard]] bool UEOpEquals(const FFaerieGridShape& Other) const;

	// A single cell.
	static FFaerieGridShape Square1;

private:
	// Internal rotation util
	[[nodiscard]] FFaerieGridShape RotateAngle(float AngleDegrees) const;
};

/*
 * A view of a FFaerieGridShape.
 */
struct FAERIEINVENTORYCONTENT_API FFaerieGridShapeView
{
	TArrayView<FIntPoint> Points;

	FFaerieGridShapeView(FFaerieGridShape& Shape)
	  : Points(Shape.Points) {}

	bool IsValid() const;
	FIntPoint GetSize() const;
	FIntRect GetBounds() const;
	FIntPoint GetShapeCenter() const;
	FIntPoint GetShapeAverageCenter() const;
	bool IsSymmetrical() const;
	bool Contains(const FIntPoint& Position) const;
	bool Overlaps(const FFaerieGridShapeView& Other) const;

	void TranslateInline(const FIntPoint& Position);
	[[nodiscard]] FFaerieGridShapeView Translate(const FIntPoint& Position) const;

	void RotateInline(ESpatialItemRotation Rotation);
	[[nodiscard]] FFaerieGridShapeView Rotate(ESpatialItemRotation Rotation) const;

	void RotateAroundInline_90(const FIntPoint& PivotPoint);
	[[nodiscard]] FFaerieGridShapeView RotateAround_90(const FIntPoint& PivotPoint) const;
	void RotateAroundInline_180(const FIntPoint& PivotPoint);
	[[nodiscard]] FFaerieGridShapeView RotateAround_180(const FIntPoint& PivotPoint) const;
	void RotateAroundInline_270(const FIntPoint& PivotPoint);
	[[nodiscard]] FFaerieGridShapeView RotateAround_270(const FIntPoint& PivotPoint) const;

	void RotateAroundCenterInline();
	[[nodiscard]] FFaerieGridShapeView RotateAroundCenter() const;

	void NormalizeInline();
	[[nodiscard]] FFaerieGridShapeView Normalize() const;

	[[nodiscard]] bool UEOpEquals(const FFaerieGridShapeView& Other) const;
};

/*
 * A const view of a FFaerieGridShape.
 */
struct FAERIEINVENTORYCONTENT_API FFaerieGridShapeConstView
{
	TConstArrayView<FIntPoint> Points;

	FFaerieGridShapeConstView() = default;

	FFaerieGridShapeConstView(const FFaerieGridShape& Shape)
	  : Points(Shape.Points) {}

	bool IsValid() const;
	FIntPoint GetSize() const;
	FIntRect GetBounds() const;
	FIntPoint GetShapeCenter() const;
	FIntPoint GetShapeAverageCenter() const;
	bool IsSymmetrical() const;
	bool Contains(const FIntPoint& Position) const;
	bool Overlaps(const FFaerieGridShapeConstView& Other) const;

	[[nodiscard]] FFaerieGridShape Copy() const;

	[[nodiscard]] bool UEOpEquals(const FFaerieGridShapeConstView& Other) const;
};