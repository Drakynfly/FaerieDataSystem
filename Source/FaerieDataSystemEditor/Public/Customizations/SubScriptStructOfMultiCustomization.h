// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"
#include "Templates/SharedPointer.h"

class IPropertyHandle;
class UClass;

namespace Faerie::Editor
{
	/**
	 * Customizes a SubScriptStructOf reference to look like a UScriptStruct* property
	 */
	class FSubScriptStructOfMultiCustomization : public IPropertyTypeCustomization
	{
	public:
		static TSharedRef<IPropertyTypeCustomization> MakeInstance()
		{
			return MakeShared<FSubScriptStructOfMultiCustomization>();
		}

		/** IPropertyTypeCustomization interface */
		virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> InPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
		virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> InPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	private:
		/** @return The struct currently set on this reference */
		const UScriptStruct* HandleGetScriptStruct() const;
		/** Set the struct used by this reference */
		void HandleSetScriptStruct(const UScriptStruct* newStruct);

		/** Handle to the property being customized */
		TSharedPtr<IPropertyHandle> PropertyHandle;
	};
}
