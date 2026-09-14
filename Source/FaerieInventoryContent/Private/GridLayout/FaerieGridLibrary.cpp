// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieGridLibrary.h"
#include "GridLayout/SpatialTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieGridLibrary)

FFaerieGridShape UFaerieGridLibrary::RotateShape(const FFaerieGridShape& InShape, const EFaerieSpatialItemRotation Rotation)
{
	FFaerieGridShape ShapeCopy = InShape;
	ShapeCopy.Rotate(Rotation).Normalize();
	return ShapeCopy;
}

FIntPoint UFaerieGridLibrary::GetSize(const FFaerieGridShape& InShape)
{
	return InShape.GetSize();
}
