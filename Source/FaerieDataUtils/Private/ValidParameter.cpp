// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ValidParameter.h"
#include "FaerieDataUtilsLog.h"

namespace Faerie::Private
{
	void ReportInvalid()
	{
		UE_LOGF(LogFaerieDataUtils, Fatal, "Invalid value assigned to TValid");
		for (;;);
	}

	void CheckLoadingValid(FArchive& Ar)
	{
		if (Ar.IsLoading())
		{
			UE_LOGF(LogFaerieDataUtils, Fatal, "Invalid value assigned to TValid while reading from archive '%ls'", *Ar.GetArchiveName());
		}
	}
}
