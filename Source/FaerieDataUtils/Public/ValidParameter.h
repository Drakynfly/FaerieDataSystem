// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "DebuggingFlags.h"

class FArchive;

namespace Faerie
{
#if FAERIE_DEBUG
	namespace Private
	{
#if DO_CHECK
		[[noreturn]] CA_NO_RETURN FAERIEDATAUTILS_API void ReportInvalid();
		FAERIEDATAUTILS_API void CheckLoadingValid(FArchive& Ar);
#endif // DO_CHECK

		template <typename T>
		concept CHasIsValid_FunctionCheck = requires(const T& Value)
		{
			{ Value.IsValid() } -> UE::CSameAs<bool>;
		};

		template <typename T>
		struct THasIsValid
		{
			static constexpr bool Value = false;
		};

		template <CHasIsValid_FunctionCheck T>
		struct THasIsValid<T>
		{
			static constexpr bool Value = true;
		};

		template <typename T>
		struct THasIsValid<T*>
		{
			static constexpr bool Value = THasIsValid<std::remove_const_t<typename TRemoveReference<T>::Type>>::Value;
		};

		template <typename T>
		struct THasIsValid<T&>
		{
			static constexpr bool Value = THasIsValid<std::remove_const_t<typename TRemoveReference<T>::Type>>::Value;
		};

		template<typename T>
		concept CHasIsValid = THasIsValid<T>::Value;
	}

	template <Private::CHasIsValid T>
	struct TValid;

	namespace Private
	{
		template <typename... ArgTypes>
		struct TIsTValidParam
		{
			static constexpr bool Value = false;
		};

		template <typename T>
		struct TIsTValidParam<TValid<T>>
		{
			static constexpr bool Value = true;
		};

		template <typename... ArgTypes>
		inline constexpr bool TIsTValidParam_V = TIsTValidParam<ArgTypes...>::Value;

		template <typename T>
		struct TValidBase
		{
			using Sub = TValid<T>;

			// Allow implicit conversion to the wrapped value, for compatibility with non-TValid APIs
			operator T() const &
			{
				return ((Sub*)this)->Val;
			}
			operator T() &&
			{
				return (T&&)((Sub*)this)->Val;
			}

			// Allow dereferencing, if it's supported
			const T& operator->() const
			{
				return ((Sub*)this)->Val;
			}
		};

		template <typename T>
		struct TValidBase<T*>
		{
			using Sub = TValid<T*>;

			// Allow implicit conversion to the wrapped value, for compatibility with non-TValid APIs
		//UE_NOTNULL_FUNCTION_NON_NULL_RETURN_START
			operator T*() const
		//UE_NOTNULL_FUNCTION_NON_NULL_RETURN_END
			{
				return ((Sub*)this)->Val;
			}

			// Allow dereferencing, if it's supported
		//UE_NOTNULL_FUNCTION_NON_NULL_RETURN_START
			T* operator->() const
		//UE_NOTNULL_FUNCTION_NON_NULL_RETURN_END
			{
				return ((Sub*)this)->Val;
			}
		};

		template <typename T>
		struct TValidBase<T&>
		{
			using Sub = TValid<T&>;

			// Allow implicit conversion to the wrapped value, for compatibility with non-TValid APIs
			operator T&() const &
			{
				return ((Sub*)this)->Val;
			}
			operator T&() &&
			{
				return (T&&)((Sub*)this)->Val;
			}

			// Allow dereferencing, if it's supported
			const T& operator->() const
			{
				return ((Sub*)this)->Val;
			}
		};
	}

	/*
	 * Modeled after TNotNull<>, TValid<> is a template that checks at construction that it's inner member is valid.
	 * Usually used with struct parameters.
	 */
	template <Private::CHasIsValid T>
	struct TValid : Private::TValidBase<T>
	{
	private:
		using Super = Private::TValidBase<T>;
		friend Super;

		T Val;

	public:
		template <Private::CHasIsValid>
		friend struct TValid;

		// Prevent default construction and construction/assignment against nullptr
		TValid() = delete;
		TValid(TYPE_OF_NULLPTR) = delete;
		TValid& operator=(TYPE_OF_NULLPTR) = delete;
		~TValid() = default;

		// Allow direct construction of the inner value
		template <typename... ArgTypes>
			requires (std::is_constructible_v<T, ArgTypes...> && !Private::TIsTValidParam_V<std::decay_t<ArgTypes>...>)
		explicit(!TIsImplicitlyConstructible_V<T, ArgTypes...>) TValid(ArgTypes&&... Args)
			: Val((ArgTypes&&)Args...)
		{
#if DO_CHECK
			if (!Val.IsValid())
			{
				Private::ReportInvalid();
			}
#endif
		}

		/////////////////////////////////////////////////
		// Start - intrusive TOptional<TValid> state //
		/////////////////////////////////////////////////
		static constexpr bool bHasIntrusiveUnsetOptionalState = HasIntrusiveUnsetOptionalState<T>();
		using IntrusiveUnsetOptionalStateType = TValid;

		UE_NODEBUG [[nodiscard]] TValid(FIntrusiveUnsetOptionalState Tag)
			requires(bHasIntrusiveUnsetOptionalState)
			: Val(Tag)
		{
		}
		UE_NODEBUG [[nodiscard]] bool operator==(FIntrusiveUnsetOptionalState Tag) const
			requires(bHasIntrusiveUnsetOptionalState)
		{
			return Val == Tag;
		}
		///////////////////////////////////////////////
		// End - intrusive TOptional<TValid> state //
		///////////////////////////////////////////////

		// Allow conversions
		template <
			typename OtherType
			UE_REQUIRES(std::is_convertible_v<const OtherType&, T>)
		>
		TValid(const TValid<OtherType>& Rhs)
			: Val(Rhs.Val)
		{
		}
		template <
			typename OtherType
			UE_REQUIRES(std::is_convertible_v<OtherType&&, T>)
		>
		TValid(TValid<OtherType>&& Rhs)
			: Val((OtherType&&)Rhs.Val)
		{
		}

		// Forward copying and moving to the inner type
		TValid(TValid&&) = default;
		TValid(const TValid&) = default;
		TValid& operator=(TValid&&) = default;
		TValid& operator=(const TValid&) = default;

		// Disallow testing for nullness - implicitly or explicitly
		operator bool() const = delete;

		// Allow dereferencing, if it's supported
		decltype(auto) operator*() const
		{
			return *Val;
		}

		// Allow function call syntax, if it's supported
		template <typename... ArgTypes>
		auto operator()(ArgTypes&&... Args) const -> decltype(this->Val((ArgTypes&&)Args...))
		{
			return Val((ArgTypes&&)Args...);
		}

		// Allow hashing, if it's supported
		friend uint32 GetTypeHash(const TValid& Valid)
		{
			return GetTypeHashHelper(Valid.Val);
		}

		// Allow comparison, if it's supported
		template <typename U>
		[[nodiscard]] auto UEOpEquals(const TValid<U>& Rhs) const -> decltype(Val == Rhs.Val)
		{
			return Val == Rhs.Val;
		}
		template <typename U>
		[[nodiscard]] auto UEOpEquals(const U& Rhs) const -> decltype(Val == Rhs)
		{
			static_assert(!std::is_same_v<U, TYPE_OF_NULLPTR>, "Comparing a TValid to nullptr is illegal");
			return Val == Rhs;
		}

		// Disallow comparison against nullptr
		bool UEOpEquals(TYPE_OF_NULLPTR Rhs) const = delete;
	};

	// Allow serialization, if it's supported
	template <typename T>
	auto operator<<(FArchive& Ar, TValid<T>& Val) -> decltype(Ar << (T&)Val)
	{
		if constexpr (std::is_void_v<decltype(Ar << (T&)Val)>)
		{
			Ar << (T&)Val;
#if DO_CHECK
			if (!(T&)Val.IsValid())
			{
				Private::CheckLoadingValid(Ar);
			}
#endif
		}
		else
		{
			decltype(auto) Result = Ar << (T&)Val;
#if DO_CHECK
			if (!(T&)Val.IsValid())
			{
				Private::CheckLoadingValid(Ar);
			}
#endif
			return Result;
		}
	}

	// Gets the inner value of a TValid without requiring a cast and repeat its inner type.
	template <typename T>
	[[nodiscard]] T ValidGet(const TValid<T>& Valid)
	{
		return Valid;
	}
	template <typename T>
	[[nodiscard]] T ValidGet(TValid<T>&& Valid)
	{
		return (TValid<T>&&)Valid;
	}
#else
	template <typename T>
	using TValid = T;

	// Gets the inner value of a TValid without requiring a cast and repeat its inner type.
	template <typename T>
	[[nodiscard]] T&& ValidGet(T&& TValid)
	{
		return (T&&)TValid;
	}
#endif
}
