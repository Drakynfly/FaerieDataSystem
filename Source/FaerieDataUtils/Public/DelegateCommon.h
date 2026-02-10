// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

template <typename TDelegate>
struct TAutoDelegate
{
	TAutoDelegate(UObject* Obj, const FName Function)
	{
		Delegate.BindUFunction(Obj, Function);
	}

	TDelegate Delegate;
};

// Create a Blueprint delegate from a static function
#define AUTO_DELEGATE_STATIC(Type, Class, Function) TAutoDelegate<Type>(GetMutableDefault<Class>(), GET_MEMBER_NAME_CHECKED(Class, Function)).Delegate

#define DYNAMIC_TO_NATIVE(NativeType, Delegate) NativeType::CreateUFunction(const_cast<UObject*>(Delegate.GetUObject()), Delegate.GetFunctionName())

// Convert a Blueprint delegate to a TScriptDelegate
#define DYNAMIC_TO_SCRIPT(Callback) TAutoDelegate<TScriptDelegate<>>(const_cast<UObject*>(Callback.GetUObject()), Callback.GetFunctionName()).Delegate

// Convert a Blueprint delegate to a c++ lambda
#define DYNAMIC_TO_LAMBDA(Function) [Dynamic_Delegate = Function](auto... Args)\
	{\
		return Dynamic_Delegate.Execute(Args...);\
	}