// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GridLayout/SpatialTypes.h"
#include "GridLayout/BitMatrix.h"

#include "Math/IntRect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpatialTypes)

FFaerieGridShape FFaerieGridShape::Square1(TArray<FIntPoint>{0, 0});

// Because FFaerieGridShape, FFaerieGridShapeView, and FFaerieGridShapeConstView use the same implementation for most
// of their functions, we can write them once as a template, and avoid lots of copied boilerplate.
namespace Faerie::Shape
{
	namespace
	{
		template <typename T>
		bool IsValid(const T& Shape)
		{
			return !Shape.Points.IsEmpty();
		}

		template <typename T, typename OtherT>
		bool UEOpEquals(const T& Shape, const OtherT& Other)
		{
			// Mismatching point numbers; auto-fail
			if (Shape.Points.Num() != Other.Points.Num()) return false;

			// @todo points have to be found individually, instead of just comparing the arrays, because the same points are not guaranteed to be in the same order.
			// to fix this, Points would have to be sorted. until then, this is really slow!!
			for (auto&& Point : Shape.Points)
			{
				if (!Other.Points.Contains(Point)) return false;
			}

			return true;
		}

		template <typename T>
		FIntPoint GetSize(const T& Shape)
		{
			if (Shape.Points.IsEmpty())
			{
				return FIntPoint::ZeroValue;
			}

			FIntPoint Size{TNumericLimits<int32>::Min()};
			for (auto&& Point : Shape.Points)
			{
				Size = Size.ComponentMax(Point);
			}

			// Add one to account for 0-indexing of points
			return Size + FIntPoint(1);
		}

		template <typename T>
		FIntRect GetBounds(const T& Shape)
		{
			if (Shape.Points.IsEmpty())
			{
				return FIntRect{0, 0};
			}

			FIntRect Bounds{ TNumericLimits<int32>::Max(), TNumericLimits<int32>::Min() };

			for (auto&& Point : Shape.Points)
			{
				Bounds.Include(Point);
			}

			return Bounds;
		}

		template <typename T>
		FIntPoint GetShapeCenter(const T& Shape)
		{
			return Shape.GetSize() / 2;
		}

		template <typename T>
		FIntPoint GetShapeAverageCenter(const T& Shape)
		{
			if (Shape.Points.IsEmpty())
			{
				return FIntPoint::ZeroValue;
			}

			// To offset for 0-indexed points, initialize Sum with Points Num.
			// This is equivalent to adding 1,1 to every point.
			FIntPoint Sum(Shape.Points.Num());
			for (const FIntPoint& Point : Shape.Points)
			{
				Sum += Point;
			}

			return Sum / Shape.Points.Num();
		}

		template <typename T>
		bool IsSymmetrical(const T& Shape)
		{
			if (Shape.Points.IsEmpty())
			{
				return true;
			}

			// Create shape copy to compare against
			FFaerieGridShape ShapeCopy = Shape.Copy();
			ShapeCopy.RotateAround_90(ShapeCopy.GetShapeCenter());
			ShapeCopy.Normalize();
			// Compare the shapes
			return UEOpEquals(ShapeCopy, Shape);
		}

		template <typename T>
		bool Contains(const T& Shape, const FIntPoint& Position)
		{
			// @todo if we make Points sorted, then we can replace this with a binary search, instead of linear
			return Shape.Points.Contains(Position);
		}

		template <typename T, typename OtherT>
		bool Overlaps(const T& Shape, const OtherT& Other)
		{
			TSet<FIntPoint> SeenPoints;
			SeenPoints.Append(Shape.Points);

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

		template <typename T>
		FFaerieGridShape Copy(const T& Shape)
		{
			return FFaerieGridShape{TArray<FIntPoint>(Shape.Points)};
		}

		template <typename T>
		T& Translate(T& Shape, const FIntPoint& Position)
		{
			for (FIntPoint& Coord : Shape.Points)
			{
				Coord += Position;
			}
			return Shape;
		}

		template <typename T>
		T& RotateAround_90(T& Shape, const FIntPoint& PivotPoint)
		{
			for (FIntPoint& Point : Shape.Points)
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
			return Shape;
		}

		template <typename T>
		T& RotateAround_180(T& Shape, const FIntPoint& PivotPoint)
		{
			for (FIntPoint& Point : Shape.Points)
			{
				// Rebase to pivot
				Point -= PivotPoint;

				// Flip
				Point *= -1;

				// Remove rebase
				Point += PivotPoint;
			}
			return Shape;
		}

		template <typename T>
		T& RotateAround_270(T& Shape, const FIntPoint& PivotPoint)
		{
			for (FIntPoint& Point : Shape.Points)
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
			return Shape;
		}

		template <typename T>
		T& RotateAroundCenter(T& Shape)
		{
			if (!Shape.Points.IsEmpty())
			{
				// Use existing rotation logic with calculated center
				RotateAround_90(Shape, GetShapeCenter(Shape));
			}
			return Shape;
		}

		template <typename T>
		T& Normalize(T& Shape)
		{
			if (Shape.Points.IsEmpty())
			{
				return Shape;
			}

			FIntPoint Min(TNumericLimits<int32>::Max());

			for (const FIntPoint& Point : Shape.Points)
			{
				Min = Min.ComponentMin(Point);
			}

			for (FIntPoint& Point : Shape.Points)
			{
				Point -= Min;
			}
			return Shape;
		}
	}
}

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
	return Faerie::Shape::IsValid(*this);
}

FIntPoint FFaerieGridShape::GetSize() const
{
	return Faerie::Shape::GetSize(*this);
}

FIntRect FFaerieGridShape::GetBounds() const
{
	return Faerie::Shape::GetBounds(*this);
}

FIntPoint FFaerieGridShape::GetShapeCenter() const
{
	return Faerie::Shape::GetShapeCenter(*this);
}

FIntPoint FFaerieGridShape::GetIndexedShapeCenter() const
{
	return (GetSize() + FIntPoint{-1}) / 2;
}

FIntPoint FFaerieGridShape::GetShapeAverageCenter() const
{
	return Faerie::Shape::GetShapeAverageCenter(*this);
}

bool FFaerieGridShape::IsSymmetrical() const
{
	return Faerie::Shape::IsSymmetrical(*this);
}

Faerie::Extensions::FBitMatrix FFaerieGridShape::ToMatrix() const
{
	const FIntRect Bounds = GetBounds();
	const FIntPoint Size = Bounds.Size() + 1; // @todo explain the +1

	// Create square matrix
	Faerie::Extensions::FBitMatrix BitMatrix(FMath::Max(Size.X, Size.Y));

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

TArray<FIntPoint> FFaerieGridShape::MatrixToPoints(const Faerie::Extensions::FBitMatrix& Matrix, const FIntPoint Origin)
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

void FFaerieGridShape::RotateMatrixClockwise(Faerie::Extensions::FBitMatrix& Matrix, const EFaerieSpatialItemRotation Rotation)
{
	if (Matrix.GetDim() <= 1 ||
		Rotation == EFaerieSpatialItemRotation::None ||
		Rotation == EFaerieSpatialItemRotation::MAX)
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
	return Faerie::Shape::Contains(*this, Position);
}

bool FFaerieGridShape::Overlaps(const FFaerieGridShape& Other) const
{
	return Faerie::Shape::Overlaps(*this, Other);
}

FFaerieGridShape& FFaerieGridShape::Translate(const FIntPoint& Position)
{
	return Faerie::Shape::Translate(*this, Position);
}

// @todo why is this different from FFaerieGridShapeView::Rotate. they should be the same. what was this suppose to fix exactly?
FFaerieGridShape& FFaerieGridShape::Rotate(const EFaerieSpatialItemRotation Rotation)
{
	if (Rotation == EFaerieSpatialItemRotation::None) return *this;

	Faerie::Extensions::FBitMatrix Matrix = ToMatrix();
	RotateMatrixClockwise(Matrix, Rotation);

	FFaerieGridShape ShapeCopy = *this;
	ShapeCopy.Normalize();
	const FIntPoint Size = ShapeCopy.GetSize();

	const FIntPoint OriginalSize = {Size.X, Size.Y};
	const FIntPoint NewSize = {Size.Y, Size.X};
	const int32 Multiplier = NewSize.X > OriginalSize.Y ? 1 : -1;

	const FIntPoint OriginOffset = {
		((NewSize.Y - OriginalSize.Y) / 2) * Multiplier,
		((NewSize.X - OriginalSize.X) / 2) * Multiplier
	};

	Points = MatrixToPoints(Matrix, OriginOffset);
	return *this;
}

FFaerieGridShape& FFaerieGridShape::RotateAround_90(const FIntPoint& PivotPoint)
{
	return Faerie::Shape::RotateAround_90(*this, PivotPoint);
}

FFaerieGridShape& FFaerieGridShape::RotateAround_180(const FIntPoint& PivotPoint)
{
	return Faerie::Shape::RotateAround_180(*this, PivotPoint);
}

FFaerieGridShape& FFaerieGridShape::RotateAround_270(const FIntPoint& PivotPoint)
{
	return Faerie::Shape::RotateAround_270(*this, PivotPoint);
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

FFaerieGridShape& FFaerieGridShape::RotateAroundCenter()
{
	return Faerie::Shape::RotateAroundCenter(*this);
}

FFaerieGridShape& FFaerieGridShape::Normalize()
{
	return Faerie::Shape::Normalize(*this);
}

FFaerieGridShape FFaerieGridShape::Copy() const
{
	return Faerie::Shape::Copy(*this);
}

bool FFaerieGridShape::UEOpEquals(const FFaerieGridShape& Other) const
{
	return Faerie::Shape::UEOpEquals(*this, Other);
}

bool FFaerieGridShapeView::IsValid() const
{
	return Faerie::Shape::IsValid(*this);
}

FIntPoint FFaerieGridShapeView::GetSize() const
{
	return Faerie::Shape::GetSize(*this);
}

FIntRect FFaerieGridShapeView::GetBounds() const
{
	return Faerie::Shape::GetBounds(*this);
}

FIntPoint FFaerieGridShapeView::GetShapeCenter() const
{
	return Faerie::Shape::GetShapeCenter(*this);
}

FIntPoint FFaerieGridShapeView::GetShapeAverageCenter() const
{
	return Faerie::Shape::GetShapeAverageCenter(*this);
}

bool FFaerieGridShapeView::IsSymmetrical() const
{
	return Faerie::Shape::IsSymmetrical(*this);
}

bool FFaerieGridShapeView::Contains(const FIntPoint& Position) const
{
	return Faerie::Shape::Contains(*this, Position);
}

bool FFaerieGridShapeView::Overlaps(const FFaerieGridShapeView& Other) const
{
	return Faerie::Shape::Overlaps(*this, Other);
}

FFaerieGridShapeView& FFaerieGridShapeView::Translate(const FIntPoint& Position)
{
	return Faerie::Shape::Translate(*this, Position);
}

FFaerieGridShapeView& FFaerieGridShapeView::Rotate(const EFaerieSpatialItemRotation Rotation)
{
	switch (Rotation)
	{
	case EFaerieSpatialItemRotation::Ninety:
		return RotateAround_90(GetShapeCenter());
	case EFaerieSpatialItemRotation::One_Eighty:
		return RotateAround_180(GetShapeCenter());
	case EFaerieSpatialItemRotation::Two_Seventy:
		return RotateAround_270(GetShapeCenter());
	case EFaerieSpatialItemRotation::None:
	case EFaerieSpatialItemRotation::MAX:
	default:
		return *this;
	}
}

FFaerieGridShapeView& FFaerieGridShapeView::RotateAround_90(const FIntPoint& PivotPoint)
{
	return Faerie::Shape::RotateAround_90(*this, PivotPoint);
}

FFaerieGridShapeView& FFaerieGridShapeView::RotateAround_180(const FIntPoint& PivotPoint)
{
	return Faerie::Shape::RotateAround_180(*this, PivotPoint);
}

FFaerieGridShapeView& FFaerieGridShapeView::RotateAround_270(const FIntPoint& PivotPoint)
{
	return Faerie::Shape::RotateAround_270(*this, PivotPoint);
}

FFaerieGridShapeView& FFaerieGridShapeView::RotateAroundCenter()
{
	return Faerie::Shape::RotateAroundCenter(*this);
}

FFaerieGridShapeView& FFaerieGridShapeView::Normalize()
{
	return Faerie::Shape::Normalize(*this);
}

FFaerieGridShape FFaerieGridShapeView::Copy() const
{
	return Faerie::Shape::Copy(*this);
}

bool FFaerieGridShapeView::UEOpEquals(const FFaerieGridShapeView& Other) const
{
	return Faerie::Shape::UEOpEquals(*this, Other);
}

bool FFaerieGridShapeConstView::IsValid() const
{
	return Faerie::Shape::IsValid(*this);
}

FIntPoint FFaerieGridShapeConstView::GetSize() const
{
	return Faerie::Shape::GetSize(*this);
}

FIntRect FFaerieGridShapeConstView::GetBounds() const
{
	return Faerie::Shape::GetBounds(*this);
}

FIntPoint FFaerieGridShapeConstView::GetShapeCenter() const
{
	return Faerie::Shape::GetShapeCenter(*this);
}

FIntPoint FFaerieGridShapeConstView::GetShapeAverageCenter() const
{
	return Faerie::Shape::GetShapeAverageCenter(*this);
}

bool FFaerieGridShapeConstView::IsSymmetrical() const
{
	return Faerie::Shape::IsSymmetrical(*this);
}

bool FFaerieGridShapeConstView::Contains(const FIntPoint& Position) const
{
	return Faerie::Shape::Contains(*this, Position);
}

bool FFaerieGridShapeConstView::Overlaps(const FFaerieGridShapeConstView& Other) const
{
	return Faerie::Shape::Overlaps(*this, Other);
}

FFaerieGridShape FFaerieGridShapeConstView::Copy() const
{
	return Faerie::Shape::Copy(*this);
}

bool FFaerieGridShapeConstView::UEOpEquals(const FFaerieGridShapeConstView& Other) const
{
	return Faerie::Shape::UEOpEquals(*this, Other);
}