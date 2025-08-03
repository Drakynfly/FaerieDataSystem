// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Containers/Array.h"

namespace Faerie
{
	class FBitMatrix
	{
	public:
		FBitMatrix(int32 Dim);

		int32 GetDim() const { return Dim; }

		void Set(int32 X, int32 Y, bool Value);

		bool Get(int32 X, int32 Y) const;

		void Transpose();
		void Reverse();

	private:
		int32 Dim;

		TArray<uint32> Data;
	};
}
