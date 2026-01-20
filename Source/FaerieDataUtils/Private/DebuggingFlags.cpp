// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "DebuggingFlags.h"

#if FAERIE_DEBUG
namespace Faerie::Debug
{
	TAutoConsoleVariable<bool> CVarEnableWriteLockTracking(
    	TEXT("fae.EnableEnsuresForWriteLocks"),
    	false,
    	TEXT("Enable to track down bugs in WriteLocks of various scoped handles in FDS."));
}
#endif