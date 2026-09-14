// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

// This file contains macros to semi-automate implementation of virtual struct machinery.
// These are used to construct one-shot macros that implement boilerplate for commonly derived structs.
// Usage is modeled after Epic's design of END_UI_TAG_DECL which extends FGameplayTag.
// Note that I don't particularly like that I have to do this, but since UScriptStruct cannot otherwise be easily
// customized, this is the best option.

// Implement a function called GetScriptStruct to return the StaticStruct for this type.
#define FAERIE_IMPL_GetScriptStruct() public: virtual const UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

/*
 * Helper to specialize TStructOpsTypeTraits for a type. Paired with FAERIE_IMPL_TStructOpsTypeTraits_END.
 * In between the two macros should be the flags overridden, like so:
	FAERIE_IMPL_TStructOpsTypeTraits_BEGIN(Type)\
	WithPostSerialize = true,\
	FAERIE_IMPL_TStructOpsTypeTraits_END(Type)
 */
#define FAERIE_IMPL_TStructOpsTypeTraits_BEGIN(Type)\
template<> struct TStructOpsTypeTraits<Type> : public TStructOpsTypeTraitsBase2<Type>\
{\
	enum\
	{

// Second half of FAERIE_IMPL_TStructOpsTypeTraits_BEGIN.
#define FAERIE_IMPL_TStructOpsTypeTraits_END(Type)\
};