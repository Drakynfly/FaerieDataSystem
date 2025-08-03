// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentEditorSettings.h"
#include "DurabilityGradeAsset.h"
#include "Misc/App.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentEditorSettings)

FName UFaerieEquipmentEditorSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#if WITH_EDITOR
TArray<FString> UFaerieEquipmentEditorSettings::GetDebugInfoForCCM(const float CCM) const
{
	TArray<FString> InfoStrings;

	for (auto&& Grade : EditorInfoGradeList)
	{
		auto&& GradePtr = Grade.LoadSynchronous();

		const float WeightForGrade = GradePtr->DefaultWeightPerCCM * CCM;

		InfoStrings.Add(FString::Printf(TEXT("%s: %f (%i)"),
			*GradePtr->GetName(), GradePtr->DefaultWeightPerCCM, static_cast<int32>(WeightForGrade)));
	}

	return InfoStrings;
}
#endif