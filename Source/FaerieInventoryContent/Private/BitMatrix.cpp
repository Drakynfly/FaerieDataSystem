// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BitMatrix.h"

namespace Faerie::Extensions
{
	FBitMatrix::FBitMatrix(const int32 Dim)
	  : Dim(Dim)
	{
		// Calculate how many uint32s we need to store all bits
		const int32 NumInts = (Dim * Dim + 31) / 32;
		Data.SetNum(NumInts, EAllowShrinking::No);
	}

	void FBitMatrix::Set(const int32 X, const int32 Y, const bool Value)
	{
		const int32 Index = Y * Dim + X;
		const int32 IntIndex = Index / 32;
		const int32 BitIndex = Index % 32;

		if (Value)
		{
			Data[IntIndex] |= (1u << BitIndex);
		}
		else
		{
			Data[IntIndex] &= ~(1u << BitIndex);
		}
	}

	bool FBitMatrix::Get(const int32 X, const int32 Y) const
	{
		const int32 Index = Y * Dim + X;
		const int32 IntIndex = Index / 32;
		const int32 BitIndex = Index % 32;
		return (Data[IntIndex] & (1u << BitIndex)) != 0;
	}

	void FBitMatrix::Transpose()
	{
		for (int32 i = 0; i < Dim; i++)
		{
			for (int32 j = i + 1; j < Dim; j++)
			{
				const bool Temp = Get(i, j);
				Set(i, j, Get(j, i));
				Set(j, i, Temp);
			}
		}
	}

	void FBitMatrix::Reverse()
	{
		for (int32 Row = 0; Row < Dim; Row++)
		{
			for (int32 Col = 0; Col < Dim / 2; Col++)
			{
				const bool Temp = Get(Col, Row);
				const int32 Inv = Dim - 1 - Col;
				Set(Col, Row, Get(Inv, Row));
				Set(Inv, Row, Temp);
			}
		}
	}
}