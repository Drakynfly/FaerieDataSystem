// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemGenerationConfigCustomization.h"

#include "DetailLayoutBuilder.h"
#include "ItemGeneratorConfig.h"
#include "FaerieItemPool.h"

TSharedRef<IDetailCustomization> FItemGenerationConfigCustomization::MakeInstance()
{
	return MakeShared<FItemGenerationConfigCustomization>();
}

void FItemGenerationConfigCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	/*
	auto&& PoolProp = DetailBuilder.GetProperty(UItemGenerationDriver::GetMemberName_DropPool());

	UObject* PoolObj;
	PoolProp->GetValue(PoolObj);
	if (IsValid(PoolObj))
	{
		auto&& DropPoolProp = PoolProp->GetChildHandle(UFaerieItemPool::GetMemberName_DropPool());
		DetailBuilder.AddPropertyToCategory(DropPoolProp);
	}
	*/
}