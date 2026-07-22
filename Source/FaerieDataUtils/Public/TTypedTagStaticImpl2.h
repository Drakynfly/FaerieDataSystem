// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

// @todo This is copied from the implementation of TTypedTagStaticImpl in the CommonUI plugin. If moved to core gameplay tags later use that version instead

/**
 * Implementation utility for typed subclasses of FGameplayTag.
 * Implemented here instead of directly within the macro to make debugging possible.
 */
template <typename TagT>
class TTypedTagStaticImpl2
{
	friend TagT;

	static TagT AddNativeTag(const FString& TagBody, const FString& Comment)
	{
		if (!ensure(!TagBody.IsEmpty()))
		{
			return TagT();
		}

		FString TagStr;
		FString RootTagStr = FString::Printf(TEXT("%s."), TagT::GetRootTagStr());
		if (!TagBody.StartsWith(RootTagStr))
		{
			TagStr = RootTagStr + TagBody;
		}
		else
		{
			TagStr = TagBody;
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
			ensureAlwaysMsgf(false, TEXT("Passed unnecessary prefix [%s] when creating a tag of type [%s] with the body [%s]"),
				*RootTagStr, TNameOf<TagT>::GetName(), *TagBody);
#endif
		}

		return UGameplayTagsManager::Get().AddNativeGameplayTag(FName(*TagStr), Comment);
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/** Intended for console commands/cheats: not for shipping code! */
	static inline TagT FindFromString_DebugOnly(const FString& PartialTagName)
	{
		return UGameplayTagsManager::Get().FindGameplayTagFromPartialString_Slow(PartialTagName);
	}
#endif

	static bool ExportTextItem(const TagT& Tag, FString& ValueStr, int32 PortFlags)
	{
		ValueStr += Tag.GetTagName().ToString();
		return true;
	}

	static TagT TryConvert(FGameplayTag VanillaTag, bool bChecked)
	{
		if (VanillaTag.MatchesTag(StaticImpl.RootTag))
		{
			return TagT(VanillaTag);
		}
		else if (VanillaTag.IsValid() && bChecked)
		{
			check(false);
		}
		return TagT();
	}

	TTypedTagStaticImpl2()
	{
		OnLastChanceToAddNativeTagsHandle = UGameplayTagsManager::CallOrRegister_OnAddNativeTagsDelegate(
			FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
			{
				StaticImpl.RootTag = UGameplayTagsManager::Get().AddNativeGameplayTag(TagT::GetRootTagStr());
			}));
	}

	~TTypedTagStaticImpl2()
	{
		UGameplayTagsManager::UnregisterNativeTagDelegate(OnLastChanceToAddNativeTagsHandle);
	}

	TagT RootTag;
	static TTypedTagStaticImpl2 StaticImpl;
	FDelegateHandle OnLastChanceToAddNativeTagsHandle;
};

template <typename TagT>
TTypedTagStaticImpl2<TagT> TTypedTagStaticImpl2<TagT>::StaticImpl;

// Intended to be the absolute last thing in the definition of a UI tag
#define END_TAG_DECL2(TagType, TagRoot)	\
public:	\
	TagType() {}	\
	static TagType GetRootTag() { return TTypedTagStaticImpl2<TagType>::StaticImpl.RootTag; }	\
	static TagType TryConvert(FGameplayTag FromTag) { return TTypedTagStaticImpl2<TagType>::TryConvert(FromTag, false); }	\
	static TagType ConvertChecked(FGameplayTag FromTag) { return TTypedTagStaticImpl2<TagType>::TryConvert(FromTag, true); }	\
	static TagType AddNativeTag(const FString& TagBody, const FString& Comment) { return TTypedTagStaticImpl2<TagType>::AddNativeTag(TagBody, Comment); }	\
	bool ExportTextItem(FString& ValueStr, const TagType& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const	\
	{	\
		return TTypedTagStaticImpl2<TagType>::ExportTextItem(*this, ValueStr, PortFlags);	\
	}	\
protected:	\
	TagType(FGameplayTag Tag) { TagName = Tag.GetTagName(); }	\
	static const TCHAR* GetRootTagStr() { return TagRoot; }	\
	friend class TTypedTagStaticImpl2<TagType>;	\
};	\
Expose_TNameOf(TagType)	\
template<>	\
struct TStructOpsTypeTraits<TagType> : public TStructOpsTypeTraitsBase2<TagType>	\
{	\
	enum	\
	{	\
		WithNetSerializer = true,	\
		WithNetSharedSerialization = true,	\
		WithPostSerialize = true,	\
		WithStructuredSerializeFromMismatchedTag = true,	\
		WithExportTextItem = true,	\
		WithImportTextItem = true	\
	};