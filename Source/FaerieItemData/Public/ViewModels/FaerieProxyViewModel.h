// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MVVMViewModelBase.h"
#include "FaerieProxyViewModel.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class FAERIEITEMDATA_API UFaerieProxyViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UE_REWRITE int32 GetStackCount() const { return StackCount; }

protected:
	void CheckItemCount(int32 NewCount);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ProxyViewModel")
	int32 StackCount = 0;
};
