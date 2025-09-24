// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "SpatialTypes.h"

#include "BitMatrix.h"
#include "FaerieInventoryContentLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpatialTypes)

FFaerieGridShape FFaerieGridShape::Square1(TArray<FIntPoint>{0, 0});

FFaerieGridShape FFaerieGridShape::MakeSquare(const int32 Size)
{
	return MakeRect(Size, Size);
}

FFaerieGridShape FFaerieGridShape::MakeRect(const int32 Height, const int32 Width)
{
	FFaerieGridShape OutShape;
	OutShape.Points.Reserve(Height * Width);
	for (int32 x = 0; x < Height; ++x)
	{
		for (int32 y = 0; y < Width; ++y)
		{
			OutShape.Points.Add(FIntPoint(x, y));
		}
	}
	return OutShape;
}

bool FFaerieGridShape::IsValid() const
{
	return !Points.IsEmpty();
}

FIntPoint FFaerieGridShape::GetSize() const
{
	if (Points.IsEmpty())
	{
		return 0;
	}

	FIntPoint Size{TNumericLimits<int32>::Min()};
	for (auto&& Point : Points)
	{
		Size = Size.ComponentMax(Point);
	}

	// Add one to account for 0-indexing of points
	return Size + FIntPoint(1);
}

FIntRect FFaerieGridShape::GetBounds() const
{
	if (Points.IsEmpty())
	{
		return FIntRect{0, 0};
	}

	FIntRect Bounds{ TNumericLimits<int32>::Max(), TNumericLimits<int32>::Min() };

	for (auto&& Point : Points)
	{
		Bounds.Include(Point);
	}

	return Bounds;
}

FIntPoint FFaerieGridShape::GetShapeCenter() const
{
	return GetSize() / 2;
}

FIntPoint FFaerieGridShape::GetIndexedShapeCenter() const
{
	return (GetSize() + FIntPoint{-1}) / 2;
}

FIntPoint FFaerieGridShape::GetShapeAverageCenter() const
{
	if (Points.IsEmpty())
	{
		return FIntPoint::ZeroValue;
	}

	// To offset for 0-indexed points, initialize Sum with Points Num.
	// This is equivalent to adding 1,1 to every point.
	FIntPoint Sum(Points.Num());
	for (const FIntPoint& Point : Points)
	{
		Sum += Point;
	}

	return Sum / Points.Num();
}

bool FFaerieGridShape::IsSymmetrical() const
{
	if (Points.IsEmpty())
	{
		return true;
	}

	// Create shape copy to compare against
	FFaerieGridShape ShapeCopy = *this;
	ShapeCopy.RotateAroundInline_90(ShapeCopy.GetShapeCenter());
	ShapeCopy.NormalizeInline();
	// Compare the shapes
	return ShapeCopy == *this;
}

Faerie::FBitMatrix FFaerieGridShape::ToMatrix() const
{
	const FIntRect Bounds = GetBounds();
	const FIntPoint Size = Bounds.Size() + 1; // @todo explain the +1

	// Create square matrix
	Faerie::FBitMatrix BitMatrix(FMath::Max(Size.X, Size.Y));

	// Only apply padding to the smaller dimension
	const FIntPoint Padding {
		(Size.Y > Size.X) ? (Size.Y - Size.X) / 2 : 0,
		(Size.X > Size.Y) ? (Size.X - Size.Y) / 2 : 0
	};

	// Set bits for points with appropriate padding
	for (const FIntPoint& Point : Points)
	{
		const FIntPoint Coord = Point - Bounds.Min + Padding;
		BitMatrix.Set(Coord.X, Coord.Y, true);
	}

	return BitMatrix;
}

TArray<FIntPoint> FFaerieGridShape::MatrixToPoints(const Faerie::FBitMatrix& Matrix, const FIntPoint Origin)
{
	TArray<FIntPoint> NewPoints;

	FIntPoint Min(Matrix.GetDim());
	FIntPoint Max(-1);

	for (int32 Row = 0; Row < Matrix.GetDim(); ++Row)
	{
		for (int32 Col = 0; Col < Matrix.GetDim(); ++Col)
		{
			if (Matrix.Get(Col, Row))
			{
				Min = Min.ComponentMin({Col, Row});
				Max = Max.ComponentMax({Col, Row});
				NewPoints.Add(FIntPoint(Col, Row));
			}
		}
	}

	for (FIntPoint& Point : NewPoints)
	{
		Point = (Point - Min) + Origin;
	}

	return NewPoints;
}

void FFaerieGridShape::RotateMatrixClockwise(Faerie::FBitMatrix& Matrix, const ESpatialItemRotation Rotation)
{
	if (Matrix.GetDim() <= 1 ||
		Rotation == ESpatialItemRotation::None ||
		Rotation == ESpatialItemRotation::MAX)
	{
		return;
	}

	const int32 NumRotations = static_cast<int32>(Rotation);

	for (int32 i = 0; i < NumRotations; ++i)
	{
		// Transpose then reverse for 90-degree clockwise rotation
		Matrix.Transpose();
		Matrix.Reverse();
	}
}

bool FFaerieGridShape::Contains(const FIntPoint& Position) const
{
	return Points.Contains(Position);
}

bool FFaerieGridShape::Overlaps(const FFaerieGridShape& Other) const
{
	TSet<FIntPoint> SeenPoints;
	SeenPoints.Append(Points);

	for (const FIntPoint& OtherPoint : Other.Points)
	{
		bool AlreadySeen;
		SeenPoints.Add(OtherPoint, &AlreadySeen);
		if (AlreadySeen)
		{
			return true;
		}
	}

	return false;
}

void FFaerieGridShape::TranslateInline(const FIntPoint& Position)
{
	for (FIntPoint& Coord : Points)
	{
		Coord += Position;
	}
}

FFaerieGridShape FFaerieGridShape::Translate(const FIntPoint& Position) const
{
	FFaerieGridShape OutShape = *this;
	OutShape.TranslateInline(Position);
	return OutShape;
}

void FFaerieGridShape::RotateInline(const ESpatialItemRotation Rotation, const bool Reset)
{
	if (!Reset && Rotation == ESpatialItemRotation::None) return;

	Faerie::FBitMatrix Matrix = ToMatrix();
	RotateMatrixClockwise(Matrix, Rotation);

	// Get Size when rotated
	auto ShapeCopy = *this;
	ShapeCopy.Points = MatrixToPoints(Matrix, 0);
	ShapeCopy.NormalizeInline();
	const FIntPoint Size = ShapeCopy.GetSize();

	const FIntPoint OriginalSize = {Size.X, Size.Y};
	const FIntPoint NewSize = {Size.Y, Size.X};
	const auto Multiplier = NewSize.X > OriginalSize.Y ? 1 : -1;

	const FIntPoint OriginOffset = {
		((NewSize.Y - OriginalSize.Y) / 2) * Multiplier,
		((NewSize.X - OriginalSize.X) / 2) * Multiplier
	};

	// Convert back to points
	Points = MatrixToPoints(Matrix, OriginOffset);
}

FFaerieGridShape FFaerieGridShape::Rotate(const ESpatialItemRotation Rotation, const bool Reset) const
{
	if (!Reset && Rotation == ESpatialItemRotation::None) return *this;

	Faerie::FBitMatrix Matrix = ToMatrix();
	RotateMatrixClockwise(Matrix, Rotation);

	FFaerieGridShape ShapeCopy;
	ShapeCopy.Points = MatrixToPoints(Matrix, 0);
	ShapeCopy.NormalizeInline();
	const FIntPoint Size = ShapeCopy.GetSize();

	const FIntPoint OriginalSize = {Size.X, Size.Y};
	const FIntPoint NewSize = {Size.Y, Size.X};

	const auto Multiplier = NewSize.X > OriginalSize.Y ? 1 : -1;

	const FIntPoint OriginOffset = {
		((NewSize.Y - OriginalSize.Y) / 2) * Multiplier,
		((NewSize.X - OriginalSize.X) / 2) * Multiplier
	};

	UE_LOG(LogFaerieInventoryContent, Warning, TEXT("Current Diff: X: %d Y: %d"), OriginOffset.X, OriginOffset.Y);
	FFaerieGridShape OutShape;
	OutShape.Points = MatrixToPoints(Matrix, OriginOffset);
	return OutShape;
}

void FFaerieGridShape::RotateAroundInline_90(const FIntPoint& PivotPoint)
{
	for (FIntPoint& Point : Points)
	{
		// Rebase to pivot
		Point -= PivotPoint;

		// Trade places
		Swap(Point.X, Point.Y);

		// Flip X - Clockwise
		Point.X *= -1;

		// Remove rebase
		Point += PivotPoint;
	}
}

FFaerieGridShape FFaerieGridShape::RotateAround_90(const FIntPoint& PivotPoint) const
{
	FFaerieGridShape NewShape = *this;
	NewShape.RotateAroundInline_90(PivotPoint);
	return NewShape;
}

void FFaerieGridShape::RotateAroundInline_180(const FIntPoint& PivotPoint)
{
	for (FIntPoint& Point : Points)
	{
		// Rebase to pivot
		Point -= PivotPoint;

		// Flip
		Point *= -1;

		// Remove rebase
		Point += PivotPoint;
	}
}

FFaerieGridShape FFaerieGridShape::RotateAround_180(const FIntPoint& PivotPoint) const
{
	FFaerieGridShape NewShape = *this;
	NewShape.RotateAroundInline_180(PivotPoint);
	return NewShape;
}

void FFaerieGridShape::RotateAroundInline_270(const FIntPoint& PivotPoint)
{
	for (FIntPoint& Point : Points)
	{
		// Rebase to pivot
		Point -= PivotPoint;

		// Trade places
		Swap(Point.X, Point.Y);

		// Flip Y - Clockwise
		Point.Y *= -1;

		// Remove rebase
		Point += PivotPoint;
	}
}

FFaerieGridShape FFaerieGridShape::RotateAround_270(const FIntPoint& PivotPoint) const
{
	FFaerieGridShape NewShape = *this;
	NewShape.RotateAroundInline_270(PivotPoint);
	return NewShape;
}

FFaerieGridShape FFaerieGridShape::RotateAngle(const float AngleDegrees) const
{
	FFaerieGridShape NewShape = *this;

	// Get center point
	const FIntPoint Center = GetShapeCenter();

	// Convert angle to radians
	const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);

	// Calculate sine and cosine
	const float CosTheta = FMath::Cos(AngleRadians);
	const float SinTheta = FMath::Sin(AngleRadians);

	for (FIntPoint& Point : NewShape.Points)
	{
		// Translate to origin
		const FIntPoint Translated = Point - Center;

		// Rotate
		const float RotatedX = Translated.X * CosTheta - Translated.Y * SinTheta;
		const float RotatedY = Translated.X * SinTheta + Translated.Y * CosTheta;

		// Translate back and round
		Point.X = FMath::RoundToInt(RotatedX + Center.X);
		Point.Y = FMath::RoundToInt(RotatedY + Center.Y);
	}

	return NewShape;
}

void FFaerieGridShape::RotateAroundCenterInline()
{
	if (Points.IsEmpty())
	{
		return;
	}

	// Use existing rotation logic with calculated center
	RotateAroundInline_90(GetShapeCenter());
}

FFaerieGridShape FFaerieGridShape::RotateAroundCenter() const
{
	FFaerieGridShape NewShape = *this;
	NewShape.RotateAroundCenterInline();
	return NewShape;
}

void FFaerieGridShape::NormalizeInline()
{
	if (Points.IsEmpty())
	{
		return;
	}

	FIntPoint Min(TNumericLimits<int32>::Max());

	for (const FIntPoint& Point : Points)
	{
		Min = Min.ComponentMin(Point);
	}

	for (FIntPoint& Point : Points)
	{
		Point -= Min;
	}
}

FFaerieGridShape FFaerieGridShape::Normalize() const
{
	FFaerieGridShape NewShape = *this;
	NewShape.NormalizeInline();
	return NewShape;
}

bool operator==(const FFaerieGridShape& Lhs, const FFaerieGridShape& Rhs)
{
	// Mismatching point numbers; auto-fail
	if (Lhs.Points.Num() != Rhs.Points.Num()) return false;

	// @todo points have to be found individually, instead of just comparing the arrays, because the same points are not guaranteed to be in the same order.
	// to fix this, Points would have to be sorted. until then, this is really slow!!
	for (auto&& Point : Lhs.Points)
	{
		if (!Rhs.Points.Contains(Point)) return false;
	}

	return true;
}

// @todo refactor these into templates!

bool FFaerieGridShapeView::IsValid() const
{
	return !Points.IsEmpty();
}

FIntPoint FFaerieGridShapeView::GetSize() const
{
	if (Points.IsEmpty())
	{
		return 0;
	}

	FIntPoint Size{TNumericLimits<int32>::Min()};
	for (auto&& Point : Points)
	{
		Size = Size.ComponentMax(Point);
	}

	return Size;
}


FIntRect FFaerieGridShapeView::GetBounds() const
{
	if (Points.IsEmpty())
	{
		return FIntRect{0, 0};
	}

	FIntRect Bounds{ TNumericLimits<int32>::Max(), TNumericLimits<int32>::Min() };

	for (auto&& Point : Points)
	{
		Bounds.Include(Point);
	}

	return Bounds;
}

FIntPoint FFaerieGridShapeView::GetShapeCenter() const
{
	return GetSize() / 2;
}

FIntPoint FFaerieGridShapeView::GetShapeAverageCenter() const
{
	if (Points.IsEmpty())
	{
		return FIntPoint::ZeroValue;
	}

	// To offset for 0-indexed points, initialize Sum with Points Num.
	// This is equivalent to adding 1,1 to every point.
	FIntPoint Sum(Points.Num());
	for (const FIntPoint& Point : Points)
	{
		Sum += Point;
	}

	return Sum / Points.Num();
}

bool FFaerieGridShapeView::IsSymmetrical() const
{
	if (Points.IsEmpty())
	{
		return true;
	}

	// Create shape copy to compare against
	FFaerieGridShapeView ShapeCopy = *this;
	ShapeCopy.RotateAroundInline_90(ShapeCopy.GetShapeCenter());
	ShapeCopy.NormalizeInline();
	// Compare the shapes
	return ShapeCopy == *this;
}

bool FFaerieGridShapeView::Contains(const FIntPoint& Position) const
{
	return Points.Contains(Position);
}

bool FFaerieGridShapeView::Overlaps(const FFaerieGridShapeView& Other) const
{
	TSet<FIntPoint> SeenPoints;
	SeenPoints.Append(Points);

	for (const FIntPoint& OtherPoint : Other.Points)
	{
		bool AlreadySeen;
		SeenPoints.Add(OtherPoint, &AlreadySeen);
		if (AlreadySeen)
		{
			return true;
		}
	}

	return false;
}

void FFaerieGridShapeView::TranslateInline(const FIntPoint& Position)
{
	for (FIntPoint& Coord : Points)
	{
		Coord += Position;
	}
}

FFaerieGridShapeView FFaerieGridShapeView::Translate(const FIntPoint& Position) const
{
	FFaerieGridShapeView OutShape = *this;
	OutShape.TranslateInline(Position);
	return OutShape;
}

void FFaerieGridShapeView::RotateInline(const ESpatialItemRotation Rotation)
{
	switch (Rotation)
	{
	case ESpatialItemRotation::Ninety:
		RotateAroundInline_90(GetShapeCenter());
		break;
	case ESpatialItemRotation::One_Eighty:
		RotateAroundInline_180(GetShapeCenter());
		break;
	case ESpatialItemRotation::Two_Seventy:
		RotateAroundInline_270(GetShapeCenter());
		break;
	case ESpatialItemRotation::None:
	case ESpatialItemRotation::MAX:
	default:
		break;
	}
}

FFaerieGridShapeView FFaerieGridShapeView::Rotate(const ESpatialItemRotation Rotation) const
{
	switch (Rotation)
	{
	case ESpatialItemRotation::Ninety:
		return RotateAround_90(GetShapeCenter());
	case ESpatialItemRotation::One_Eighty:
		return RotateAround_180(GetShapeCenter());
	case ESpatialItemRotation::Two_Seventy:
		return RotateAround_270(GetShapeCenter());
	case ESpatialItemRotation::None:
	case ESpatialItemRotation::MAX:
	default:
		return *this;
	}
}

void FFaerieGridShapeView::RotateAroundInline_90(const FIntPoint& PivotPoint)
{
	for (FIntPoint& Point : Points)
	{
		// Rebase to pivot
		Point -= PivotPoint;

		// Trade places
		Swap(Point.X, Point.Y);

		// Flip Y
		Point.Y *= -1;

		// Remove rebase
		Point += PivotPoint;
	}
}

FFaerieGridShapeView FFaerieGridShapeView::RotateAround_90(const FIntPoint& PivotPoint) const
{
	FFaerieGridShapeView NewShape = *this;
	NewShape.RotateAroundInline_90(PivotPoint);
	return NewShape;
}

void FFaerieGridShapeView::RotateAroundInline_180(const FIntPoint& PivotPoint)
{
	for (FIntPoint& Point : Points)
	{
		// Rebase to pivot
		Point -= PivotPoint;

		// Flip
		Point *= -1;

		// Remove rebase
		Point += PivotPoint;
	}
}

FFaerieGridShapeView FFaerieGridShapeView::RotateAround_180(const FIntPoint& PivotPoint) const
{
	FFaerieGridShapeView NewShape = *this;
	NewShape.RotateAroundInline_180(PivotPoint);
	return NewShape;
}

void FFaerieGridShapeView::RotateAroundInline_270(const FIntPoint& PivotPoint)
{
	for (FIntPoint& Point : Points)
	{
		// Rebase to pivot
		Point -= PivotPoint;

		// Trade places
		Swap(Point.X, Point.Y);

		// Flip X
		Point.X *= -1;

		// Remove rebase
		Point += PivotPoint;
	}
}

FFaerieGridShapeView FFaerieGridShapeView::RotateAround_270(const FIntPoint& PivotPoint) const
{
	FFaerieGridShapeView NewShape = *this;
	NewShape.RotateAroundInline_270(PivotPoint);
	return NewShape;
}

void FFaerieGridShapeView::RotateAroundCenterInline()
{
	if (Points.IsEmpty())
	{
		return;
	}

	// Use existing rotation logic with calculated center
	RotateAroundInline_90(GetShapeCenter());
}

FFaerieGridShapeView FFaerieGridShapeView::RotateAroundCenter() const
{
	FFaerieGridShapeView NewShape = *this;
	NewShape.RotateAroundCenterInline();
	return NewShape;
}

void FFaerieGridShapeView::NormalizeInline()
{
	if (Points.IsEmpty())
	{
		return;
	}

	FIntPoint Min(TNumericLimits<int32>::Max());

	for (const FIntPoint& Point : Points)
	{
		Min = Min.ComponentMin(Point);
	}

	for (FIntPoint& Point : Points)
	{
		Point -= Min;
	}
}

FFaerieGridShapeView FFaerieGridShapeView::Normalize() const
{
	FFaerieGridShapeView NewShape = *this;
	NewShape.NormalizeInline();
	return NewShape;
}

bool operator==(const FFaerieGridShapeView& Lhs, const FFaerieGridShapeView& Rhs)
{
	// Mismatching point numbers; auto-fail
	if (Lhs.Points.Num() != Rhs.Points.Num()) return false;

	// @todo points have to be found individually, instead of just comparing the arrays, because the same points are not guaranteed to be in the same order.
	// to fix this, Points would have to be sorted. until then, this is really slow!!
	for (auto&& Point : Lhs.Points)
	{
		if (!Rhs.Points.Contains(Point)) return false;
	}

	return true;
}

bool FFaerieGridShapeConstView::IsValid() const
{
	return !Points.IsEmpty();
}

FIntPoint FFaerieGridShapeConstView::GetSize() const
{
	if (Points.IsEmpty())
	{
		return 0;
	}

	FIntPoint Size{TNumericLimits<int32>::Min()};
	for (auto&& Point : Points)
	{
		Size = Size.ComponentMax(Point);
	}

	return Size;
}


FIntRect FFaerieGridShapeConstView::GetBounds() const
{
	if (Points.IsEmpty())
	{
		return FIntRect{0, 0};
	}

	FIntRect Bounds{ TNumericLimits<int32>::Max(), TNumericLimits<int32>::Min() };

	for (auto&& Point : Points)
	{
		Bounds.Include(Point);
	}

	return Bounds;
}

FIntPoint FFaerieGridShapeConstView::GetShapeCenter() const
{
	return GetSize() / 2;
}

FIntPoint FFaerieGridShapeConstView::GetShapeAverageCenter() const
{
	if (Points.IsEmpty())
	{
		return FIntPoint::ZeroValue;
	}

	// To offset for 0-indexed points, initialize Sum with Points Num.
	// This is equivalent to adding 1,1 to every point.
	FIntPoint Sum(Points.Num());
	for (const FIntPoint& Point : Points)
	{
		Sum += Point;
	}

	return Sum / Points.Num();
}

bool FFaerieGridShapeConstView::IsSymmetrical() const
{
	if (Points.IsEmpty())
	{
		return true;
	}

	// Create shape copy to compare against
	FFaerieGridShape Shape = Copy();
	Shape.RotateAroundInline_90(Shape.GetShapeCenter());
	Shape.NormalizeInline();
	// Compare the shapes
	return FFaerieGridShapeConstView(Shape) == *this;
}

bool FFaerieGridShapeConstView::Contains(const FIntPoint& Position) const
{
	return Points.Contains(Position);
}

bool FFaerieGridShapeConstView::Overlaps(const FFaerieGridShapeConstView& Other) const
{
	TSet<FIntPoint> SeenPoints;
	SeenPoints.Append(Points);

	for (const FIntPoint& OtherPoint : Other.Points)
	{
		bool AlreadySeen;
		SeenPoints.Add(OtherPoint, &AlreadySeen);
		if (AlreadySeen)
		{
			return true;
		}
	}

	return false;
}

FFaerieGridShape FFaerieGridShapeConstView::Copy() const
{
	return FFaerieGridShape{TArray<FIntPoint>(Points)};
}

bool operator==(const FFaerieGridShapeConstView& Lhs, const FFaerieGridShapeConstView& Rhs)
{
	// Mismatching point numbers; auto-fail
	if (Lhs.Points.Num() != Rhs.Points.Num()) return false;

	// @todo points have to be found individually, instead of just comparing the arrays, because the same points are not guaranteed to be in the same order.
	// to fix this, Points would have to be sorted. until then, this is really slow!!
	for (auto&& Point : Lhs.Points)
	{
		if (!Rhs.Points.Contains(Point)) return false;
	}

	return true;
}