// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/Build.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
#define FAERIE_DEBUG 1
#else
#define FAERIE_DEBUG 0
#endif

#if FAERIE_DEBUG
#include "HAL/IConsoleManager.h"

namespace Faerie::Debug
{
	FAERIEDATAUTILS_API extern TAutoConsoleVariable<bool> CVarEnableWriteLockTracking;
}
#endif