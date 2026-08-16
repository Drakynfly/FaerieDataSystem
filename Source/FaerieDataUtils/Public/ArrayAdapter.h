// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Templates/Function.h"

namespace Faerie::Utils
{
	template <typename T>
	struct TArrayAdapter
	{
		using FCallbackType = TFunction<T(int32)>;

		TArrayAdapter(const int32 Num, const FCallbackType& Callback)
		  : Count(Num), Callback(Callback) {}

		TArrayAdapter(const TArray<T>& Array UE_LIFETIMEBOUND)
		  : Count(Array.Num()), Callback([Array](const int32 Index)
			{
				return Array[Index];
			}) {}

		TArrayAdapter(TConstArrayView<T> Array UE_LIFETIMEBOUND)
		  : Count(Array.Num()), Callback([Array](const int32 Index)
			{
				return Array[Index];
			}) {}

		UE_REWRITE int32 Num() const { return Count; }
		auto operator[](const int32 Index) const { return Callback(Index); }

	private:
		int32 Count;
		FCallbackType Callback;
	};
}
