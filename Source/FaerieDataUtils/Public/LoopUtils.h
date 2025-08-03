// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

namespace Faerie
{
	// A simple enum to make iterating callbacks more legible
	enum ELoopControl
	{
		Continue,
		Stop
	};

	// A simple typedef over a TFunctionRef to make iterator functions more legible.
	template <typename... ParamTypes>
	using TLoop = const TFunctionRef<void(ParamTypes...)>&;

	// A simple typedef over a TFunctionRef to make breakable iterator functions more legible.
	template <typename... ParamTypes>
	using TBreakableLoop = const TFunctionRef<ELoopControl(ParamTypes...)>&;
}
